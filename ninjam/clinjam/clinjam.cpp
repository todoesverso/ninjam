#include <stdlib.h>
#include <memory.h>

#include <stdio.h>
#include <math.h>
#include <signal.h>
#include <float.h>

#include "../audiostream.h"
#include "../njclient.h"
#include "../../WDL/lineparse.h"

#define UNUSED(x) (void)(x)

audioStreamer *g_audio;
NJClient *g_client;
int acceptlicense = 0;

void audiostream_onunder()
{
}

void audiostream_onover()
{
}

int g_audio_enable = 0;

void audiostream_onsamples(float **inbuf, int innch, float **outbuf, int outnch,
                           int len, int srate)
{
    if (!g_audio_enable) {
        int x;
        // clear all output buffers
        for (x = 0; x < outnch; x++)
            memset(outbuf[x], 0, sizeof(float) * len);
        return;
    }
    g_client->AudioProc(inbuf, innch, outbuf, outnch, len, srate);
}

int licensecallbak(void *userData, const char *licensetext)
{
    UNUSED(userData);
    UNUSED(licensetext);
    return acceptlicense > 0;
}

int g_done = 0;

void sigfunc(int sig)
{
    UNUSED(sig);
    printf("Got Ctrl+C\n");
    g_done++;
}

void usage()
{
    printf("Usage: ninjam hostname [options]\n"
           "Options:\n"
           "  -localchannels [0..2]\n"
           "  -norecv\n"
           "  -acceptlicense\n"
           "  -user <username>\n"
           "  -pass <password>\n"
           "  -monitor <level in dB>\n"
           "  -metronome <level in dB>\n"
           "  -debuglevel [0..2]\n"
           "  -nowritelog\n");
    exit(1);
}

int main(int argc, char **argv)
{
    char *parmuser = NULL;
    char *parmpass = NULL;
    int localchannels = 1, nostatus = 0;

    float monitor = 1.0;
    printf("Ninjam v0.006 linux command line client\n");
    g_client = new NJClient;
    g_client->LicenseAgreementCallback=licensecallbak;

    if (argc < 2)
        usage();

    {
        int p;
        for (p = 2; p < argc; p++) {
            if (!stricmp(argv[p], "-localchannels")) {
                if (++p >= argc)
                    usage();
                localchannels = atoi(argv[p]);
                if (localchannels < 0 || localchannels > 2) {
                    printf
                    ("Error: -localchannels must have parameter [0..2]\n");
                    return 1;
                }
            } else if (!stricmp(argv[p], "-acceptlicense")) {
                acceptlicense = 1;
            } else if (!stricmp(argv[p], "-norecv")) {
                g_client->config_autosubscribe = 0;
            } else if (!stricmp(argv[p], "-nostatus")) {
                nostatus = 1;
            } else if (!stricmp(argv[p], "-monitor")) {
                if (++p >= argc)
                    usage();
                monitor = (float)pow(2.0, atof(argv[p]) / 6.0);
            } else if (!stricmp(argv[p], "-metronome")) {
                if (++p >= argc)
                    usage();
                g_client->config_metronome =
                    (float)pow(2.0, atof(argv[p]) / 6.0);
            } else if (!stricmp(argv[p], "-debuglevel")) {
                if (++p >= argc)
                    usage();
                g_client->config_debug_level = atoi(argv[p]);
            } else if (!stricmp(argv[p], "-user")) {
                if (++p >= argc)
                    usage();
                parmuser = argv[p];
            } else if (!stricmp(argv[p], "-pass")) {
                if (++p >= argc)
                    usage();
                parmpass = argv[p];
            } else
                usage();
        }
    }

    if (!parmuser) {
        printf("Enter username");
        exit(1);
    }

    g_audio = create_audioStreamer_JACK("JACK", audiostream_onsamples);

    if (!g_audio) {
        printf("Error opening audio\n!");
        exit(1);
    }

    printf("Opened at %dHz %d->%dch %dbps\n",
           g_audio->m_srate, g_audio->m_innch, g_audio->m_outnch,
           g_audio->m_bps);

    signal(SIGINT, sigfunc);
    JNL::open_socketlib();


    {
        FILE *fp=fopen("ninjam.config","rt");
        if (fp) {
            bool comment_state=false;
            while (!feof(fp)) {
                char buf[4096];
                buf[0]=0;
                fgets(buf,sizeof(buf),fp);
                if (!buf[0]) continue;
                if (buf[strlen(buf)-1] == '\n')
                    buf[strlen(buf)-1]=0;
                if (!buf[0]) continue;
                LineParser lp(comment_state);
                lp.parse(buf);
                switch (lp.gettoken_enum(0,"local\0master\0")) {
                case 0:
                    // process local line
                    if (lp.getnumtokens()>2) {
                        int ch=lp.gettoken_int(1);
                        int n;
                        for (n = 2; n < lp.getnumtokens()-1; n += 2) {
                            switch (lp.gettoken_enum(n,"source\0bc\0mute\0solo\0volume\0pan\0jesus\0name\0")) {
                            case 0: // source
                                g_client->SetLocalChannelInfo(ch,NULL,true,lp.gettoken_int(n+1),false,0,false,false);
                                break;
                            case 1: //broadcast
                                g_client->SetLocalChannelInfo(ch,NULL,false,false,false,0,true,!!lp.gettoken_int(n+1));
                                break;
                            case 2: //mute
                                g_client->SetLocalChannelMonitoring(ch,false,false,false,false,true,!!lp.gettoken_int(n+1),false,false);
                                break;
                            case 3: //solo
                                g_client->SetLocalChannelMonitoring(ch,false,false,false,false,false,false,true,!!lp.gettoken_int(n+1));
                                break;
                            case 4: //volume
                                g_client->SetLocalChannelMonitoring(ch,true,(float)lp.gettoken_float(n+1),false,false,false,false,false,false);
                                break;
                            case 5: //pan
                                g_client->SetLocalChannelMonitoring(ch,false,false,true,(float)lp.gettoken_float(n+1),false,false,false,false);
                                break;
                            case 6: //jesus
                                break;
                            case 7: //name
                                g_client->SetLocalChannelInfo(ch,lp.gettoken_str(n+1),false,false,false,0,false,false);
                                break;
                            default:
                                break;
                            }
                        }
                    }
                    break;
                case 1:
                    if (lp.getnumtokens()>2) {
                        int n;
                        for (n = 1; n < lp.getnumtokens()-1; n += 2) {
                            switch (lp.gettoken_enum(n,"mastervol\0masterpan\0metrovol\0metropan\0mastermute\0metromute\0")) {
                            case 0: // mastervol
                                g_client->config_mastervolume = (float)lp.gettoken_float(n+1);
                                break;
                            case 1: // masterpan
                                g_client->config_masterpan = (float)lp.gettoken_float(n+1);
                                break;
                            case 2:
                                g_client->config_metronome = (float)lp.gettoken_float(n+1);
                                break;
                            case 3:
                                g_client->config_metronome_pan = (float)lp.gettoken_float(n+1);
                                break;
                            case 4:
                                g_client->config_mastermute = !!lp.gettoken_int(n+1);
                                break;
                            case 5:
                                g_client->config_metronome_mute = !!lp.gettoken_int(n+1);
                                break;
                            default:
                                break;
                            }
                        }
                    }
                    break;
                default:
                    break;
                }


            }
            fclose(fp);
        } else {
            if (localchannels > 0) {
                g_client->SetLocalChannelInfo(0, "channel0", true, 0, false, 0,
                                              true, true);
                g_client->SetLocalChannelMonitoring(0, true, monitor, false,
                                                    0.0, false, false, false,
                                                    false);

                if (localchannels > 1) {
                    g_client->SetLocalChannelInfo(1, "channel1", true, 1,
                                                  false, 0, true, true);
                    g_client->SetLocalChannelMonitoring(1, true, monitor,
                                                        false, 0.0, false,
                                                        false, false,
                                                        false);
                }
            }
        }
    }

    printf("Connecting to %s with user %s...\n", argv[1], parmuser);
    g_client->Connect(argv[1], parmuser, parmpass);
    g_audio_enable = 1;

    int lastloopcnt = 0;
    int statuspos = 0;

    while (g_client->GetStatus() >= 0 && !g_done) {
        if (g_client->Run()) {
            struct timespec ts = { 0, 1000 * 1000 };
            nanosleep(&ts, NULL);
            if (g_client->HasUserInfoChanged()) {
                printf("\nUser, channel list:\n");
                int us = 0;
                for (;;) {
                    char *un = g_client->GetUserState(us);
                    if (!un)
                        break;
                    printf(" %s\n", un);
                    int i;
                    for (i = 0;; i++) {
                        bool sub;
                        int ch =
                            g_client->EnumUserChannels
                            (us, i++);
                        if (ch < 0)
                            break;

                        char *cn =
                            g_client->GetUserChannelState
                            (us, ch,
                             &sub);
                        if (!cn)
                            break;
                        printf
                        ("    %d: \"%s\" subscribed=%d\n",
                         ch, cn, sub ? 1 : 0);
                        ch++;
                    }
                    us++;
                }
                if (!us)
                    printf("  <no users>\n");
            }
            if (!nostatus) {
                int lc = g_client->GetLoopCount();
                if (lastloopcnt != lc) {
                    int l;
                    g_client->GetPosition(NULL, &l);
                    lastloopcnt = lc;

                    printf
                    ("\rEntering interval %d (%d samples, %.2f bpm)%20s\n",
                     lc, l, g_client->GetActualBPM(),
                     "");
                    printf("[%28s|%29s]\r[", "", "");
                    statuspos = 0;
                } else {
                    int l, p;
                    g_client->GetPosition(&p, &l);
                    p *= 60;
                    p /= l;
                    while (statuspos < p) {
                        statuspos++;
                        printf("#");
                    }
                }
            }
        }
    }

    switch (g_client->GetStatus()) {
    case NJClient::NJC_STATUS_OK:
        break;
    case NJClient::NJC_STATUS_INVALIDAUTH:
        printf("ERROR: invalid login/password\n");
        break;
    case NJClient::NJC_STATUS_CANTCONNECT:
        printf("ERROR: failed connecting to host\n");
        break;
    case NJClient::NJC_STATUS_PRECONNECT:
        printf("ERROR: failed connect\n");
        break;
    case NJClient::NJC_STATUS_DISCONNECTED:
        printf("ERROR: disconnected from host\n");
        break;
    default:
        printf("exiting on status %d\n", g_client->GetStatus());
        break;
    }

    if (g_client->GetErrorStr()[0]) {
        printf("Server gave explanation: %s\n",
               g_client->GetErrorStr());
    }

    int ret = g_client->GetStatus();
    printf("exiting on status %d\n", ret);
    printf("Shutting down\n");

    delete g_audio;
    delete g_client;

    JNL::close_socketlib();
    return ret;
}
