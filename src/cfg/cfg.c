/**
  \file   cfg.c
  \author Aleksey Lekhtin
  \date   April 5, 2024
  \brief  Config variables
*/

#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/stat.h>

#include "ads.h"
#include "atlas.h"
#include "cfg.h"
#include "dbg.h"
#include "adc.h"
#include "utils.h"
#include "epl.h"
#include "log.h"

const char *msg_esz7_not_loaded = "it seems 'esz7' module was not loaded before !!!!!!!!!! ";

//---------------------------------------------------------
/**
 * Исходные конфигурационные параметры.
 * @note Переменная глобальной видимости.
 */
static cfg_t m_cfg = {
    .logfname = "/var/log/esz7.log",
    .mode = MODE_UNKN,
    .do_dump = MODE_UNKN,
    .decimal = DECIMAL_DOT,
    .locale = "en_US",
    .lang_cli = "en",
    .lang_srv = "en",
    .system_units = UnitsMetric,
    .cmd_port = DEF_ESZ7_CMD_PORT,
    .dpt_delay = 0,
    .swap_channels12 = 0,

    // --- ATLAS ---
    .atlas_host1 = {0},
    .atlas_port11 = 0,
    .atlas_port12 = 0,

    .atlas_host2 = {0},
    .atlas_port21 = 0,
    .atlas_port22 = 0,

    .atlas_host3 = {0},
    .atlas_port31 = 0,
    .atlas_port32 = 0,

    .atlas_serno = DEF_ATLAS_SERNO,
    .atlas_proto_ver = 0,

    .adc_atlas.fpgafreq = 100000000, // Gz

    .adc_atlas.reg_mode0 = MODE0_ATLAS_DEF,
    .adc_atlas.reg_mode1 = MODE1_ATLAS_DEF,
    .adc_atlas.reg_mode3 = MODE3_ATLAS_DEF,
    .adc_atlas.reg_mode4 = MODE4_ATLAS_DEF,
    .adc_atlas.reg_ref = REF_ATLAS_DEF,
    .adc_atlas.scanperiod = 0,

    // --- APS ---
    .ads_serno = DEF_ADS_SERNO,
    .ads_cmd_port = DEF_ADS_PORT_CMD,
    .ads_data_port = DEF_ADS_PORT_DATA,

    .siu_serno = DEF_SIU2_SERNO,
    .siu_cmd_port = DEF_SIU2_PORT_CMD,
    .siu_data_port = DEF_SIU2_PORT_DATA,
    .sensor_connected[0] = true, // PRES1AVAIL
    .sensor_connected[1] = true, // PRES2AVAIL
    .sensor_connected[2] = true, // HKLDAVAIL

    .dtr_serno = DEF_DTR_SERNO,
    .dtr_cmd_port = DEF_DTR_PORT_CMD,
    .dtr_data_port = DEF_DTR_PORT_DATA,

    .adc_aps.reg_mode0 = MODE0_ADS_DEF,
    .adc_aps.reg_mode1 = MODE1_ADS_DEF,
    .adc_aps.reg_mode3 = MODE3_ADS_DEF,
    .adc_aps.reg_mode4 = MODE4_ADS_DEF,
    .adc_aps.reg_ref = REF_ADS_DEF,
    .adc_aps.scanperiod = 0,

    .adc_aps.fpgafreq = 100000000, // Gz
    .pressure[0].max_scale = 6000, // psi
    .pressure[0].pumps_min = 200,
    .pressure[0].pumps_max = 1000,
    .pressure[0].pumps_on_threshold = 100,

    .pressure[1].max_scale = 6000, // psi
    .pressure[1].pumps_min = 200,
    .pressure[1].pumps_max = 1000,
    .pressure[1].pumps_on_threshold = 100,

    // --- ATLAS WITS ---
    .wits_host = {0},
    .wits_port = DEF_WITS_PORT,

    // --- APS WITS ---
    .aps_wits_host = {0},
    .aps_wits_port = DEF_WITS_PORT,
    .aps_wits_serno = DEF_WITS_SERNO,
};

//---------------------------------------------------------
inline cfg_t *cfg()
{
    return &m_cfg;
}

//---------------------------------------------------------
/**
 * Справка о запуске
 * @param av - Список аргументов
 */
static void usage(char *av[])
{
    printf("\nUsage: %s  [options] [-d <max_npnts>] [-v] [-h]\n", basename(av[0]));
    printf("Параметры:\n"
        "    -m, --mode <1..2>         - режим работы (1 - Atlas, 2 - APS)\n"
        "    -t, --test [1..3]         - вывод данных на каналах\n"
        "                                1 - буфер sram , 2 - готовые данные, 3 - средние\n"
        "    -c, --channel [1..4]      - номер канала для отображения в тестах\n"
        "    -s, --scan <ticks/100MHz> - период опроса АЦП (в тиках)\n"
        "        --dump                - сохранение готовых пакетов в файл\n"
        "    -d <1-5>                  - отладочный вывод в терминал\n"
        "    -h                        - справка\n"
        "    -v                        - версия\n"
        "\n");
    printf("см. %s.cfg для настройки рабочей конфигурации\n\n", basename(av[0]));
}

//---------------------------------------------------------
/**
 * Разбор командной строки
 * @param ac - Количество аргументов
 * @param av - Список аргументов
 */
void parse_help_args(int ac, char *av[])
{
    for (int i = 1; i < ac; ++i) {
        if (strcasecmp(av[i], "-v") == 0 ||
            strcasecmp(av[i], "--version") == 0 ||
            strcasecmp(av[i], "/v") == 0) {
            printf("\n%s\n\n", version());
            exit(0);
        }
        if (strcasecmp(av[i], "-h") == 0 ||
            strcasecmp(av[i], "--help") == 0 ||
            strcasecmp(av[i], "/?") == 0 ||
            strcasecmp(av[i], "?") == 0) {
            printf("\n%s\n", version());
            usage(av);
            exit(0);
        }
        if (strncasecmp(av[i], "-d", 2) == 0) {
            if ((i + 1) == ac) {
                if (strlen(av[i]) == 2) {
                    debug_level = 1;
                    continue;
                }
                debug_level = strtol(av[i] + 2, NULL, 10);
                continue;
            }
            if (*av[i + 1] == '-') {
                debug_level = 1;
                continue;
            }
            debug_level = strtol(av[i + 1], NULL, 10);
            ++i;
            continue;
        }
    }
}

//---------------------------------------------------------
static void cfg_read_serial_number_file(const char *fname)
{
    const char *defserno;
    char *sernostr;
    size_t sernostr_sz;

    if (strcmp(fname, DEF_ATLAS_SERNO_FNAME) == 0) {
        defserno = DEF_ATLAS_SERNO;
        sernostr = m_cfg.atlas_serno;
        sernostr_sz = sizeof(m_cfg.atlas_serno);
    } else if (strcmp(fname, DEF_ADS_SERNO_FNAME) == 0) {
        defserno = DEF_ADS_SERNO;
        sernostr = m_cfg.ads_serno;
        sernostr_sz = sizeof(m_cfg.ads_serno);
    } else if (strcmp(fname, DEF_SIU2_SERNO_FNAME) == 0) {
        defserno = DEF_SIU2_SERNO;
        sernostr = m_cfg.siu_serno;
        sernostr_sz = sizeof(m_cfg.siu_serno);
    } else if (strcmp(fname, DEF_DTR_SERNO_FNAME) == 0) {
        defserno = DEF_DTR_SERNO;
        sernostr = m_cfg.dtr_serno;
        sernostr_sz = sizeof(m_cfg.dtr_serno);
    } else {
        return;
    }

    int fd = open(fname, O_RDONLY, 0640);
    if (fd == -1) {
        LOG(CRITICAL, "can't read serial number from '%s'. set '%s'", fname, defserno);
        DBGF(0, "can't read serial number from '%s'. set '%s'", fname, defserno);
        strncpy(sernostr, defserno, sernostr_sz - 1);
        fd = open(fname, O_CREAT | O_WRONLY, 0640);
        if (fd == -1) {
            LOG(FATAL, "can't create default serial number");
            DBG(0, "fatal error - can't create default serial number");
            return;
        }
        write(fd, sernostr, strlen(sernostr));
        write(fd, "\n", 1);
        close(fd);
        fd = open(fname, O_RDONLY, 0640);
        if (fd == -1) {
            int err = errno;
            LOG(CRITICAL, "can't read serial number from '%s'. '%s'", fname, strerror(err));
            DBGF(0, "can't read serial number from '%s'. '%s'", fname, strerror(err));
            return;
        }
    }
    if (read(fd, sernostr, sernostr_sz - 1) == -1) {
        strncpy(sernostr, defserno, sernostr_sz - 1);
        LOG(CRITICAL, "can't read serial number from '%s'. using '%s'", fname, defserno);
        DBGF(0, "can't read serial number from '%s'. using '%s'", fname, defserno);
        close(fd);
        return;
    }
    char *p = sernostr;
    while (*p) {
        if (ispunct(*p) || !isalnum(*p) || isblank(*p)) {
            memmove(p, p + 1, strlen(p));
            continue;
        }
        p++;
    }
    close(fd);
}

//---------------------------------------------------------
static void cfg_read_serial_number(work_mode_t mode)
{
    const char *fn;

    if (mode == MODE_ATLAS) {
        fn = DEF_ATLAS_SERNO_FNAME;
        cfg_read_serial_number_file(fn);
    } else if (mode == MODE_APS) {
        fn = DEF_ADS_SERNO_FNAME;
        cfg_read_serial_number_file(fn);

        fn = DEF_SIU2_SERNO_FNAME;
        cfg_read_serial_number_file(fn);

        fn = DEF_DTR_SERNO_FNAME;
        cfg_read_serial_number_file(fn);
    }
}

//---------------------------------------------------------
static void cfg_save_serial_number_file(const char *fname)
{
    char *sernostr;
    if (strcmp(fname, DEF_ATLAS_SERNO_FNAME) == 0) {
        sernostr = m_cfg.atlas_serno;
    } else if (strcmp(fname, DEF_ADS_SERNO_FNAME) == 0) {
        sernostr = m_cfg.ads_serno;
    } else if (strcmp(fname, DEF_SIU2_SERNO_FNAME) == 0) {
        sernostr = m_cfg.siu_serno;
    } else if (strcmp(fname, DEF_DTR_SERNO_FNAME) == 0) {
        sernostr = m_cfg.dtr_serno;
    } else {
        return;
    }

    int fd = open(fname, O_CREAT | O_TRUNC | O_WRONLY, 0640);
    if (fd == -1) {
        int err = errno;
        LOG(CRITICAL, "can't create file '%s' (%s)", fname, strerror(err));
        DBGF(0, "can't create file '%s' (%s)", fname, strerror(err));
        return;
    }
    char *p = sernostr;
    while (*p) {
        if (ispunct(*p) || !isalnum(*p) || isblank(*p)) {
            memmove(p, p + 1, strlen(p));
            continue;
        }
        p++;
    }
    write(fd, sernostr, strlen(sernostr));
    close(fd);
}

//---------------------------------------------------------
void cfg_save_serial_number(work_mode_t mode)
{
    if (mode == MODE_ATLAS) {
        cfg_save_serial_number_file(DEF_ATLAS_SERNO_FNAME);
    } else if (mode == MODE_APS) {
        cfg_save_serial_number_file(DEF_ADS_SERNO_FNAME);
        cfg_save_serial_number_file(DEF_SIU2_SERNO_FNAME);
        cfg_save_serial_number_file(DEF_DTR_SERNO_FNAME);
    }
}

//---------------------------------------------------------
/**
 * Создание шаблона конфигурациогнного файла для системы Atlas
 * @return OK / FAIL
 */
static bool cfg_create_default_atlas(const char *fname)
{
    time_t tim = time(NULL);
    FILE *fp = fopen(fname, "w");
    if (!fp) {
        int e = errno;
        printf("\n\nwarning - can't create file %s (%s)\n", fname, strerror(e));
        return FAIL;
    }

    char buf[4 * 1024] = {0};
    snprintf(buf, sizeof(buf) - 1,
             "# Created: %s"
             "#\n"
             "# To restore the default configuration - delete this file.\n"
             "# It will be recreated the next time app launch.\n"
             "#"
             "\n"
             "host1=192.168.1.31        # адрес куда отправлять UDP данные\n"
             "port11=%-5d              # порт #1 для отправки\n"
             "port12=%-5d              # порт #2 для отправки\n"
             "\n"
             "host2=                    # адрес куда отправлять UDP данные\n"
             "port21=%-5d              # порт #1 для отправки\n"
             "port22=%-5d              # порт #2 для отправки\n"
             "\n"
             "host3=                    # адрес куда отправлять UDP данные\n"
             "port31=%-5d              # порт #1 для отправки\n"
             "port32=%-5d              # порт #2 для отправки\n"
             "\n"
             "wits_port=%-5d           # WITS Network \n"
             "\n"
             "proto_ver=1               # 1 - pressure 1/2 + hookload, 2 - 1/2, ch:4, hkld\n"
             "\n"
             "# DO NOT EDIT!\n"
             "# The following parameters are for debugging purposes\n"
             "# and should not be used unless you know what they are for.\n"
             "\n"
             "adc_ref=%0.4f\n"
             "\n"
             "# reg_mode0=0x%02X\n"
             "# reg_mode1=0x%02X\n"
             "# reg_mode3=0x%02X\n"
             "# reg_mode4=0x%02X\n"
             "# reg_ref=0x%02X\n"
             "# scanperiod=%-9d   # default(ticks): 200000\n"
             "# fpgafreq=%-10d    # default(Gz): 100000000\n"
             "\n"
             "# dump=0            # (do not use)\n"
             "# tracemode=0       # (do not use)\n"
             "\n",
             ctime(&tim),
             DEF_ATLAS_PORT,
             DEF_ATLAS_PORT + 1,
             DEF_ATLAS_PORT,
             DEF_ATLAS_PORT + 1,
             DEF_ATLAS_PORT,
             DEF_ATLAS_PORT + 1,
             DEF_WITS_PORT,
             m_cfg.adc_ref,
             m_cfg.adc_atlas.reg_mode0,
             m_cfg.adc_atlas.reg_mode1,
             m_cfg.adc_atlas.reg_mode3,
             m_cfg.adc_atlas.reg_mode4,
             m_cfg.adc_atlas.reg_ref,
             200000,
             m_cfg.adc_atlas.fpgafreq);

    size_t nwrote = fwrite(buf, strlen(buf), 1, fp);
    fclose(fp);
    return (nwrote == 1 ? OK : FAIL);
}

//---------------------------------------------------------
/**
 * Создание шаблона конфигурациогнного файла \n
 * для системы ADS (Advanced Decoding System by APS)
 * @return OK / FAIL
 */
static bool cfg_create_default_aps(const char *fname)
{
    time_t tim = time(NULL);
    FILE *fp = fopen(fname, "w");
    if (!fp) {
        int e = errno;
        printf("\n\nwarning - can't create file %s (%s)\n", fname, strerror(e));
        return FAIL;
    }

    char buf[4 * 1024] = {0};
    snprintf(buf, sizeof(buf) - 1,
             "# Created: %s"
             "#\n"
             "# To restore the default configuration - delete this file.\n"
             "# It will be recreated the next time app launch.\n"
             "#"
             "\n"
             "ads_cmd_port=%-5d         # ADS command port\n"
             "ads_data_port=%-5d        # ADS data port\n"
             "\n"
             "ss_cmd_port=%-5d          # SureShot command port \n"
             "ss_dats_port=%-5d         # SureShot data port\n"
             "\n"
             "dtr_cmd_port=%-5d         # Depth Tracker command port \n"
             "dtr_data_port=%-5d        # Depth Tracker data port\n"
             "\n"
             "wits_port=%-5d            # WITS Network \n"
             "\n"
             "SystemUnits=M              # M or E (Metric/English)\n"
             "Decimal=dot                # decimal delimiter (dot or comma)\n"
             "\n"
             "\n"
             "# DO NOT EDIT!\n"
             "\n"
             "adc_ref=%0.4f\n"
             "\n"
             "# reg_mode0=0x%02X\n"
             "# reg_mode1=0x%02X\n"
             "# reg_mode3=0x%02X\n"
             "# reg_mode4=0x%02X\n"
             "# reg_ref=0x%02X\n"
             "# scanperiod=%-7d       # default(ticks): 250000\n"
             "# fpgafreq=%-10d      # default(Gz): 100000000\n"
             "\n"
             "# The following parameters are using if the pulse threshold is not 0\n"
             "# pulse_threshold=0xFFFF   # used for pulse inclinometer at input ADC\n"
             "# pressure1_maxscale=%-4u  # pressure max scale\n"
             "# pump1_min=%-4u           # min work pressure of pump\n"
             "# pump1_max=%-4u           # max work pressure of pump\n"
             "# pump1_on_threshold=%-4u  # pressure on threshold\n"
             "\n"
             "# pressure2_maxscale=%-4u  # \n"
             "# pump2_min=%-4u           # min work pressure of pump\n"
             "# pump2_max=%-4u           # max work pressure of pump\n"
             "# pump2_on_threshold=%-4u  # \n"
             "\n"
             "# dump=0                   # (do not use)\n"
             "# tracemode=0              # (do not use)\n"
             "\n",
             ctime(&tim),
             DEF_ADS_PORT_CMD,
             DEF_ADS_PORT_DATA,
             DEF_SIU2_PORT_CMD,
             DEF_SIU2_PORT_DATA,
             DEF_DTR_PORT_CMD,
             DEF_DTR_PORT_DATA,
             DEF_WITS_PORT,

             m_cfg.adc_ref,
             m_cfg.adc_aps.reg_mode0,
             m_cfg.adc_aps.reg_mode1,
             m_cfg.adc_aps.reg_mode3,
             m_cfg.adc_aps.reg_mode4,
             m_cfg.adc_aps.reg_ref,
             250000,
             m_cfg.adc_aps.fpgafreq,

             m_cfg.pressure[0].max_scale,
             m_cfg.pressure[0].pumps_min,
             m_cfg.pressure[0].pumps_max,
             m_cfg.pressure[0].pumps_on_threshold,

             m_cfg.pressure[1].max_scale,
             m_cfg.pressure[1].pumps_min,
             m_cfg.pressure[1].pumps_max,
             m_cfg.pressure[1].pumps_on_threshold
    );
    size_t nwrote = fwrite(buf, strlen(buf), 1, fp);
    fclose(fp);
    return (nwrote == 1 ? OK : FAIL);
}

//---------------------------------------------------------
/** Check parameters and setup defaults
 * @return OK / FAIL
*/
void cfg_check_defaults()
{
    if (m_cfg.cmd_port == 0) {
        m_cfg.cmd_port = DEF_ESZ7_CMD_PORT;
    }

    if (m_cfg.adc_atlas.fpgafreq == 0) {
        m_cfg.adc_atlas.fpgafreq = 100000000; // Gz
    }

    if (m_cfg.adc_aps.fpgafreq == 0) {
        m_cfg.adc_aps.fpgafreq = 100000000; // Gz
    }

    //-------------------------------------------
    double time_for_measu = 0; // sec
    if (m_cfg.mode == MODE_ATLAS) {
        if (m_cfg.adc_atlas.scanperiod == 0) {
            time_for_measu = 1.0; // sec
            double value = (double) m_cfg.adc_atlas.fpgafreq / atlas()->npnts500 * time_for_measu;
            m_cfg.adc_atlas.scanperiod = (uint32_t) value;
        } else {
            time_for_measu =
                    (double) m_cfg.adc_atlas.scanperiod * atlas()->npnts500 / m_cfg.adc_atlas.fpgafreq;
        }
        if (m_cfg.test_scanperiod == 0) {
            m_cfg.test_scanperiod = m_cfg.adc_atlas.scanperiod;
        }
    } else if (m_cfg.mode == MODE_APS) {
        if (m_cfg.adc_aps.scanperiod == 0) {
            time_for_measu = 0.25; // sec (100 точек за 1/4 сек =Ю 400 т/сек)
            const double value = (double) m_cfg.adc_aps.fpgafreq / ads()->data.npnts100 * time_for_measu;
            m_cfg.adc_aps.scanperiod = (uint32_t) value;
        } else {
            time_for_measu =
                    (double) m_cfg.adc_aps.scanperiod * ads()->data.npnts100 / m_cfg.adc_aps.fpgafreq;
        }
        if (m_cfg.test_scanperiod == 0) {
            m_cfg.test_scanperiod = m_cfg.adc_aps.scanperiod;
        }
    }

    m_cfg.atlas_timeshift = (int) (time_for_measu * 1000.0); // ms

    //-------------------------------------------
    if (m_cfg.ads_cmd_port == 0) {
        m_cfg.ads_cmd_port = DEF_ADS_PORT_CMD;
    }
    if (m_cfg.ads_data_port == 0) {
        m_cfg.ads_data_port = DEF_ADS_PORT_DATA;
    }

    //-------------------------------------------
    if (m_cfg.siu_cmd_port == 0) {
        m_cfg.siu_cmd_port = DEF_SIU2_PORT_CMD;
    }
    if (m_cfg.siu_data_port == 0) {
        m_cfg.siu_data_port = DEF_SIU2_PORT_DATA;
    }

    //-------------------------------------------
    if (m_cfg.dtr_cmd_port == 0) {
        m_cfg.dtr_cmd_port = DEF_DTR_PORT_CMD;
    }
    if (m_cfg.dtr_data_port == 0) {
        m_cfg.dtr_data_port = DEF_DTR_PORT_DATA;
    }

    //-------------------------------------------
    if (m_cfg.pressure[0].scale == 0) {
        m_cfg.pressure[0].scale = (double) m_cfg.pressure[0].max_scale / 0xFFFF;
    }
    if (m_cfg.pressure[1].scale == 0) {
        m_cfg.pressure[1].scale = (double) m_cfg.pressure[1].max_scale / 0xFFFF;
    }

    //-------------------------------------------
    if (!m_cfg.rawfname[0]) {
        strncpy(m_cfg.rawfname, "obj.raw", sizeof(m_cfg.rawfname) - 1);
    }

    //-------------------------------------------
    if (debug_level < 0) {
        debug_level = 0;
    }
}

//---------------------------------------------------------
/**
* Разбор параметров командной строки
* @param ac - количество аргументов командной строки
* @param av - список аргументов командной строки
* @return OK - успешно выполнено (1)
* @return FAIL - ошибка (0)
*/
bool cfg_parse_args(int ac, char *av[])
{
    for (int i = 1; i < ac; ++i) {
        if (strncasecmp(av[i], "-d", 2) == 0) {
            if ((i + 1) == ac) {
                if (strlen(av[i]) == 2) {
                    debug_level = 1;
                    continue;
                }
                debug_level = strtol(av[i] + 2, NULL, 10);
                continue;
            }
            if (av[i + 1][0] == '-') {
                debug_level = 1;
                continue;
            }
            debug_level = strtol(av[i + 1], NULL, 10);
            ++i;
        } else if (strcasecmp(av[i], "--mode") == 0 || strncasecmp(av[i], "-m", 2) == 0) {
            m_cfg.mode = MODE_UNKN;
            if ((i + 1) < ac && av[i + 1][0] != '-') {
                ++i;
                errno = 0;
                int num = strtol(av[i], NULL, 10);
                if (errno == 0) {
                    if (num == 1) {
                        m_cfg.mode = MODE_ATLAS;
                    } else if (num == 2) {
                        m_cfg.mode = MODE_APS;
                    }
                }
            }
        } else if (strcasecmp(av[i], "--scan") == 0 || strncasecmp(av[i], "-s", 2) == 0) {
            m_cfg.test_scanperiod = 0;
            if ((i + 1) < ac && av[i + 1][0] != '-') {
                ++i;
                errno = 0;
                int num = strtol(av[i], NULL, 10);
                if (errno == 0) {
                    m_cfg.test_scanperiod = num;
                }
            }
        } else if (strcasecmp(av[i], "--test") == 0 || strncasecmp(av[i], "-t", 2) == 0) {
            m_cfg.test_mode = TEST1;
            if ((i + 1) < ac && av[i + 1][0] != '-') {
                ++i;
                errno = 0;
                int num = strtol(av[i], NULL, 10);
                if (errno == 0) {
                    m_cfg.test_mode = (TEST_MODE) num;
                    if (m_cfg.test_mode > TEST3) {
                        m_cfg.test_mode = TEST3;
                    }
                }
            }
        } else if (strcasecmp(av[i], "--channel") == 0 || strncasecmp(av[i], "-c", 2) == 0) {
            m_cfg.test_channel = 0;
            if ((i + 1) < ac && av[i + 1][0] != '-') {
                ++i;
                errno = 0;
                int num = strtol(av[i], NULL, 10);
                if (errno == 0) {
                    m_cfg.test_channel = num;
                    if (num >= 0 && num <= 4) {
                        m_cfg.test_channel = num;
                    }
                }
            }
        } else if (strcasecmp(av[i], "--dump") == 0) {
            m_cfg.do_dump = 1;
        } else {
            printf("\n\n%s - unknown argument\n\n", av[i]);
            return FAIL;
        }
    }
    return OK;
}

//---------------------------------------------------------
static void cfg_parse_params_for_adc(adc_cfg_t *adc_cfg, const char *param, const char *str)
{
    uint32_t val;
    if (strcasecmp(param, "fpgafreq") == 0) {
        errno = 0;
        val = strtoul(str, NULL, 10);
        if (errno == 0) {
            adc_cfg->fpgafreq = val;
        }
    } else if (strcasecmp(param, "reg_mode0") == 0) {
        errno = 0;
        val = strtoul(str, NULL, 16);
        if (errno == 0) {
            adc_cfg->reg_mode0 = val;
        }
    } else if (strcasecmp(param, "reg_mode1") == 0) {
        errno = 0;
        val = strtoul(str, NULL, 16);
        if (errno == 0) {
            adc_cfg->reg_mode1 = val;
        }
    } else if (strcasecmp(param, "reg_mode3") == 0) {
        errno = 0;
        val = strtoul(str, NULL, 16);
        if (errno == 0) {
            adc_cfg->reg_mode3 = val;
        }
    } else if (strcasecmp(param, "reg_mode4") == 0) {
        errno = 0;
        val = strtoul(str, NULL, 16);
        if (errno == 0) {
            adc_cfg->reg_mode4 = val;
        }
    } else if (strcasecmp(param, "reg_ref") == 0) {
        errno = 0;
        val = strtoul(str, NULL, 16);
        if (errno == 0) {
            adc_cfg->reg_ref = val;
        }
    } else if (strcasecmp(param, "scanperiod") == 0) {
        errno = 0;
        val = strtoul(str, NULL, 10);
        if (errno == 0) {
            adc_cfg->scanperiod = val;
        }
    } else if (strcasecmp(param, "adc_ref") == 0) {
        errno = 0;
        const double v = strtod(str, NULL);
        if (errno == 0) {
            m_cfg.adc_ref = v;
            m_cfg.adc_scale = v / (double)((1u << 23u) - 1u);
        }
    }
}

//---------------------------------------------------------
static void cfg_parse_params(char *text, work_mode_t mode)
{
    uint32_t val;
    char *pline;
    char *available = text;

    while (1) {
        if (!available) {
            break;
        }
        if (available == text) {
            pline = text;
        } else {
            pline = available + 1;
        }

        available = strchr(pline, '\n');
        if (available) {
            *available = '\0';
        }

        char *p = strchr(pline, '#');
        if (p) {
            *p = '\0';
        }
        alltrim(pline);

        char *param = pline;
        char *str = NULL;
        p = strchr(pline, '=');
        if (!p) {
            continue;
        }

        *p = '\0';
        p++;
        str = p;
        alltrim(param);
        alltrim(str);
        DBGF(4, "'%s' = '%s'", param, str);
        // Common SIU settings
        if (strcasecmp(param, "lang_cli") == 0) {
            strncpy(m_cfg.lang_cli, str, sizeof(m_cfg.lang_cli) - 1);
        } else if (strcasecmp(param, "lang_srv") == 0) {
            strncpy(m_cfg.lang_srv, str, sizeof(m_cfg.lang_srv) - 1);
        } else if (strcasecmp(param, "SystemUnits") == 0) {
            if (strcasecmp(str, "M") == 0 || strcasecmp(str, "Metric") == 0) {
                m_cfg.system_units = UnitsMetric;
            } else {
                m_cfg.system_units = UnitsEnglish;
            }
        } else if (strcasecmp(param, "decimal") == 0) {
            if (strcasecmp(str, "dot") == 0) {
                m_cfg.decimal = DECIMAL_DOT;
            } else if (strcasecmp(str, "comma") == 0) {
                m_cfg.decimal = DECIMAL_COMMA;
            } else if (strcasecmp(str, "coma") == 0 ||
                       strcasecmp(str, "comm") == 0 ||
                       strcasecmp(str, "cmma") == 0) {
                LOG(ERROR, "incorrect parameter cfg::decimal::%s (did you meant 'comma' ?)", str);
                DBGF(0, "incorrect parameter cfg::decimal::%s (did you meant 'comma' ?)\n", str);
            }
        } else if (strcasecmp(param, "dump") == 0) {
            m_cfg.do_dump = (work_mode_t) MODE_UNKN;
            errno = 0;
            val = strtol(str, NULL, 10);
            if (errno == 0) {
                if (val == MODE_ATLAS || val == MODE_APS) {
                    m_cfg.do_dump = (work_mode_t) val;
                }
            }
        } else if (strcasecmp(param, "tracemode") == 0) {
            m_cfg.shortmsg = 0;
            errno = 0;
            val = strtol(str, NULL, 10);
            if (errno == 0) {
                m_cfg.shortmsg = (int) val;
            }
        } else if (strcasecmp(param, "esz7_cmd_port") == 0) {
            m_cfg.cmd_port = (int) strtol(str, NULL, 10);
        }

        //---------------------------------------
        if (mode == MODE_ATLAS) {
            if (strcasecmp(param, "host1") == 0) {
                strncpy(m_cfg.atlas_host1, str, sizeof(m_cfg.atlas_host1) - 1);
            } else if (strcasecmp(param, "port11") == 0) {
                m_cfg.atlas_port11 = (int) strtol(str, NULL, 10);
            } else if (strcasecmp(param, "port12") == 0) {
                m_cfg.atlas_port12 = (int) strtol(str, NULL, 10);
            } else if (strcasecmp(param, "host2") == 0) {
                strncpy(m_cfg.atlas_host2, str, sizeof(m_cfg.atlas_host2) - 1);
            } else if (strcasecmp(param, "port21") == 0) {
                m_cfg.atlas_port21 = (int) strtol(str, NULL, 10);
            } else if (strcasecmp(param, "port22") == 0) {
                m_cfg.atlas_port22 = (int) strtol(str, NULL, 10);
            } else if (strcasecmp(param, "host3") == 0) {
                strncpy(m_cfg.atlas_host3, str, sizeof(m_cfg.atlas_host3) - 1);
            } else if (strcasecmp(param, "port31") == 0) {
                m_cfg.atlas_port31 = (int) strtol(str, NULL, 10);
            } else if (strcasecmp(param, "port32") == 0) {
                m_cfg.atlas_port32 = (int) strtol(str, NULL, 10);
            } else if (strcasecmp(param, "proto_ver") == 0) {
                m_cfg.atlas_proto_ver = (int) strtol(str, NULL, 10);
            } else {
                cfg_parse_params_for_adc(&m_cfg.adc_atlas, param, str);
            }
        } else if (mode == MODE_APS) {
            if (strcasecmp(param, "ads_cmd_port") == 0) {
                m_cfg.ads_cmd_port = (int) strtol(str, NULL, 10);
            } else if (strcasecmp(param, "ads_data_port") == 0) {
                m_cfg.ads_data_port = (int) strtol(str, NULL, 10);
            }

            if (strcasecmp(param, "ss_cmd_port") == 0) {
                m_cfg.siu_cmd_port = (int) strtol(str, NULL, 10);
            } else if (strcasecmp(param, "ss_data_port") == 0) {
                m_cfg.siu_data_port = (int) strtol(str, NULL, 10);
            }

            if (strcasecmp(param, "dtr_cmd_port") == 0) {
                m_cfg.dtr_cmd_port = (int) strtol(str, NULL, 10);
            } else if (strcasecmp(param, "dtr_data_port") == 0) {
                m_cfg.dtr_data_port = (int) strtol(str, NULL, 10);
            } else if (strcasecmp(param, "pulse_threshold") == 0) {
                int base = 10;
                if (strncasecmp(str, "0x", 2) == 0) {
                    base = 16;
                }
                errno = 0;
                val = strtol(str, NULL, base);
                if (errno == 0) {
                    m_cfg.pulsethreshold = val;
                }
            } else if (strcasecmp(param, "pressure1_maxscale") == 0) {
                errno = 0;
                val = strtoul(str, NULL, 10);
                if (errno == 0) {
                    m_cfg.pressure[0].max_scale = val;
                }
            } else if (strcasecmp(param, "pump1_min") == 0) {
                errno = 0;
                val = strtoul(str, NULL, 10);
                if (errno == 0) {
                    m_cfg.pressure[0].pumps_min = val;
                }
            } else if (strcasecmp(param, "pump1_max") == 0) {
                errno = 0;
                val = strtoul(str, NULL, 10);
                if (errno == 0) {
                    m_cfg.pressure[0].pumps_max = val;
                }
            } else if (strcasecmp(param, "pump1_threshold") == 0) {
                errno = 0;
                val = strtoul(str, NULL, 10);
                if (errno == 0) {
                    m_cfg.pressure[0].pumps_on_threshold = val;
                }
            } else if (strcasecmp(param, "pressure2_maxscale") == 0) {
                errno = 0;
                val = strtoul(str, NULL, 10);
                if (errno == 0) {
                    m_cfg.pressure[1].max_scale = val;
                }
            } else if (strcasecmp(param, "pump2_min") == 0) {
                errno = 0;
                val = strtoul(str, NULL, 10);
                if (errno == 0) {
                    m_cfg.pressure[1].pumps_min = val;
                }
            } else if (strcasecmp(param, "pump2_max") == 0) {
                errno = 0;
                val = strtoul(str, NULL, 10);
                if (errno == 0) {
                    m_cfg.pressure[1].pumps_max = val;
                }
            } else if (strcasecmp(param, "pump2_threshold") == 0) {
                errno = 0;
                val = strtoul(str, NULL, 10);
                if (errno == 0) {
                    m_cfg.pressure[1].pumps_on_threshold = val;
                }
            } else {
                cfg_parse_params_for_adc(&m_cfg.adc_aps, param, str);
            }
        }
    } // while (1);
}

//---------------------------------------------------------
static bool cfg_parse_file(const char *fname)
{
    // 1.
    int fd = open(fname, O_RDONLY, 0640);
    if (fd == -1) {
        int e = errno;
        if (e != ENOENT) {
            printf("\n\nERROR: Can't open file '%s'\n%s\n", fname, strerror(e));
            return FAIL;
        }

        // 2.1
        if (m_cfg.mode == MODE_ATLAS) {
            if (!cfg_create_default_atlas(fname)) {
                return FAIL;
            }
        } else if (m_cfg.mode == MODE_APS) {
            if (!cfg_create_default_aps(fname)) {
                return FAIL;
            }
        }

        // 2.2
        printf("-- created config file template '%s'\n", fname);
        fd = open(fname, O_RDONLY, 0640);
        if (fd == -1) {
            printf("\n\nERROR: Can't read config file '%s'\n", fname);
            return FAIL;
        }
    }

    //-------------------------------------------
    // 3.
    char text[4 * 1024] = {0};
    size_t nread = read(fd, text, sizeof(text) - 1);
    close(fd);
    if (nread == 0) {
        int e = errno;
        printf("\n\nERROR: Can't read config file - '%s'\n(%d) %s\n",
               fname, e, strerror(e));
        return FAIL;
    }

    // 4.
    cfg_parse_params(text, m_cfg.mode);
    return OK;
}

//---------------------------------------------------------
const char *cfg_mode_string(const work_mode_t mode)
{
    if (mode == MODE_ATLAS) {
        return "ATLAS";
    }
    if (mode == MODE_APS) {
        return "APS";
    }
    if (mode == MODE_UNKN) {
        return "UNKN";
    }
    return "?";
}

//---------------------------------------------------------
void cfg_print_configuration()
{
    if (debug_level == 0) {
        return;
    }
    DBGF(1, "cfg.dbg:                %d", debug_level);
    DBGF(1, "cfg.do_dump:            %d", m_cfg.do_dump);
    DBGF(1, "cfg.decimal:            %s%s",
         (m_cfg.decimal == DECIMAL_DOT ? "dot (.)" : ""),
         (m_cfg.decimal == DECIMAL_COMMA ? "comma (,)" : ""));
    DBGF(1, "cfg.lang_cli:           %s (SureShot)", m_cfg.lang_cli);
    DBGF(1, "cfg.lang_srv:           %s (SIU)     ", m_cfg.lang_srv);
    DBGF(1, "cfg.sensor_pressure1:   %s", (m_cfg.sensor_connected[0] ? "ON" : "OFF"));
    DBGF(1, "cfg.sensor_pressure2:   %s", (m_cfg.sensor_connected[1] ? "ON" : "OFF"));
    DBGF(1, "cfg.sensor_hookload:    %s", (m_cfg.sensor_connected[2] ? "ON" : "OFF"));

    if (m_cfg.mode == MODE_ATLAS) {
        DBGF(1, "cfg.atlas_serno:        %s", m_cfg.atlas_serno);
        DBGF(1, "cfg.atlas_host1:        %s", m_cfg.atlas_host1);
        DBGF(1, "cfg.atlas_port11:       %u", m_cfg.atlas_port11);
        DBGF(1, "cfg.atlas_port21:       %u", m_cfg.atlas_port12);
        DBGF(1, "cfg.atlas_host2:        %s", m_cfg.atlas_host2);
        DBGF(1, "cfg.atlas_port21:       %u", m_cfg.atlas_port21);
        DBGF(1, "cfg.atlas_port22:       %u", m_cfg.atlas_port22);
        DBGF(1, "cfg.atlas_host3:        %s", m_cfg.atlas_host3);
        DBGF(1, "cfg.atlas_port31:       %u", m_cfg.atlas_port31);
        DBGF(1, "cfg.atlas_port32:       %u", m_cfg.atlas_port32);
        DBGF(1, "cfg.atlas_timeshift:    %u", m_cfg.atlas_timeshift);
        DBGF(1, "cfg.wits_port:          %u", m_cfg.wits_port);

        DBGF(1, "cfg.atlas.ref:          %0.04f", m_cfg.adc_ref);
        DBGF(1, "cfg.atlas.scale_factor: %0.20f", m_cfg.adc_scale);
        DBGF(1, "cfg.atlas_reg_mode0:    0x%02X", m_cfg.adc_atlas.reg_mode0);
        DBGF(1, "cfg.atlas_reg_mode1:    0x%02X", m_cfg.adc_atlas.reg_mode1);
        DBGF(1, "cfg.atlas_reg_mode3:    0x%02X", m_cfg.adc_atlas.reg_mode3);
        DBGF(1, "cfg.atlas_reg_mode4:    0x%02X", m_cfg.adc_atlas.reg_mode4);
        DBGF(1, "cfg.atlas_reg_ref:      0x%02X", m_cfg.adc_atlas.reg_ref);
        DBGF(1, "cfg.atlas_scanperiod:   %u    ", m_cfg.adc_atlas.scanperiod);
        DBGNL(1);
    } else {
        DBGNL(1);

        DBGF(1, "cfg.ads_serno:          %s", m_cfg.ads_serno);
        DBGF(1, "cfg.ads_cmd_port:       %u", m_cfg.ads_cmd_port);
        DBGF(1, "cfg.ads_data_port:      %u", m_cfg.ads_data_port);
        DBGNL(1);

        DBGF(1, "cfg.siu_serno:          %s", m_cfg.siu_serno);
        DBGF(1, "cfg.siu_cmd_port:       %u", m_cfg.siu_cmd_port);
        DBGF(1, "cfg.siu_data_port:      %u", m_cfg.siu_data_port);
        DBGNL(1);

        DBGF(1, "cfg.dtr_serno:          %s    ", m_cfg.dtr_serno);
        DBGF(1, "cfg.dtr_cmd_port:       %u    ", m_cfg.dtr_cmd_port);
        DBGF(1, "cfg.dtr_data_port:      %u    ", m_cfg.dtr_data_port);
        DBGNL(1);

        DBGF(1, "cfg.aps_wits_serno:     %s", m_cfg.aps_wits_serno);
        DBGF(1, "cfg.aps_wits_port:      %u", m_cfg.aps_wits_port);
        DBGNL(1);

        DBGF(1, "cfg.pressure1.maxscale: %-4u  ", m_cfg.pressure[0].max_scale);
        DBGF(1, "cfg.pressure1.scale:    %0.20f", m_cfg.pressure[0].scale);
        DBGF(1, "cfg.pump1.min:          %-4u  ", m_cfg.pressure[0].pumps_min);
        DBGF(1, "cfg.pump1.max:          %-4u  ", m_cfg.pressure[0].pumps_max);
        DBGF(1, "cfg.pump1.threshold:    %-4u  ", m_cfg.pressure[0].pumps_on_threshold);
        DBGNL(1);

        DBGF(1, "cfg.pressure2.maxscale: %-4u  ", m_cfg.pressure[1].max_scale);
        DBGF(1, "cfg.pressure2.scale:    %0.20f", m_cfg.pressure[1].scale);
        DBGF(1, "cfg.pump2.min:          %-4u  ", m_cfg.pressure[1].pumps_min);
        DBGF(1, "cfg.pump2.max:          %-4u  ", m_cfg.pressure[1].pumps_max);
        DBGF(1, "cfg.pump2.threshold:    %-4u  ", m_cfg.pressure[1].pumps_on_threshold);
        DBGNL(1);

        DBGF(1, "cfg.aps.ref:            %0.04f", m_cfg.adc_ref);
        DBGF(1, "cfg.aps.scale_factor:   %0.20f", m_cfg.adc_scale);
        DBGF(1, "cfg.aps_reg_mode0:      0x%02X", m_cfg.adc_aps.reg_mode0);
        DBGF(1, "cfg.aps_reg_mode1:      0x%02X", m_cfg.adc_aps.reg_mode1);
        DBGF(1, "cfg.aps_reg_mode3:      0x%02X", m_cfg.adc_aps.reg_mode3);
        DBGF(1, "cfg.aps_reg_mode4:      0x%02X", m_cfg.adc_aps.reg_mode4);
        DBGF(1, "cfg.aps_reg_ref:        0x%02X", m_cfg.adc_aps.reg_ref);
        DBGF(1, "cfg.aps_scanperiod:     %u    ", m_cfg.adc_aps.scanperiod);
        DBGNL(1);

        DBGF(1, "cfg.pulsethreshold:     0x%04X", m_cfg.pulsethreshold);
    }

    DBGF(1, "cfg.event_fn:           %s", epl()->file[0].fname);
    DBGF(1, "cfg.esz7_cmd_port:      %d", m_cfg.cmd_port);
    DBGF(1, "cfg.dpt_delay:          %d", m_cfg.dpt_delay);
    DBGF(1, "cfg.swap_channels12:    %d", m_cfg.swap_channels12);
    DBGNL(1);
}

//---------------------------------------------------------
bool cfg_save_esz7_status()
{
    char buf[1024] = {0};
    DBGF(3, "save '%s'", cfg()->esz7_cfg_fname);
    int fd = open(cfg()->esz7_cfg_fname, O_CREAT | O_TRUNC | O_WRONLY, 0640);
    if (fd == -1) {
        int err = errno;
        LOG(FATAL, "can't open file %s (%s)\n", cfg()->esz7_cfg_fname, strerror(err));
        DBGF(0, "\n\nERROR: can't open file %s (%s)\n", cfg()->esz7_cfg_fname, strerror(err));
        return FAIL;
    }

    snprintf(buf, sizeof(buf), "dpt_delay=%d\n", m_cfg.dpt_delay);
    DBGFNoNL(9, "%s", buf);
    write(fd, buf, strlen(buf));

    snprintf(buf, sizeof(buf), "swap_ch12=%s\n", (m_cfg.swap_channels12 ? "yes" : "no"));
    DBGFNoNL(9, "%s", buf);
    write(fd, buf, strlen(buf));

    write(fd, "\n", 1);
    close(fd);
    return OK;
}

//---------------------------------------------------------
bool cfg_read_esz7_status()
{
    DBGF(1, "read '%s'", cfg()->esz7_cfg_fname);
    if (cfg()->esz7_cfg_fname[0] == 0) {
        LOG(CRITICAL, "config file name is empty");
        DBG(0, "config file name is empty");
        return FAIL;
    }

    int fd = open(cfg()->esz7_cfg_fname, O_RDONLY, 0640);
    if (fd == -1) {
        if (errno == ENOENT) {
            LOG(CRITICAL, "file not found - %s (%s)", cfg()->esz7_cfg_fname, strerror(errno));
            DBGF(0, "file not found - %s (%s)", cfg()->esz7_cfg_fname, strerror(errno));
            cfg_save_esz7_status();
        }
        fd = open(cfg()->esz7_cfg_fname, O_RDONLY, 0640);
        if (fd == -1) {
            int err = errno;
            LOG(FATAL, "can't create file %s (%s)\n", cfg()->esz7_cfg_fname, strerror(err));
            DBGF(0, "\n\nERROR: can't create file %s (%s)\n", cfg()->esz7_cfg_fname, strerror(err));
            return FAIL;
        }
    }
    char cfgtext[4 * 1024] = {0};
    read(fd, cfgtext, sizeof(cfgtext) - 1);
    close(fd);

    // понизить все буквы "вниз"
    char *p = cfgtext;
    while (*p != '\0') {
        if (*p == '\r') {
            *p = '\n';
        } else {
            *p = tolower(*p);
        }
        p++;
    }

    // список искомыз параметров
    const char *param[] = {
        "dpt_delay",
        "swap_ch12"
    };
    size_t param_size = sizeof(param) / sizeof(param[0]);

    char *p0 = cfgtext;
    for (size_t i = 0; i < param_size; i++) {
        char *p1 = strstr(p0, param[i]);
        if (!p1) {
            continue;
        }
        p = strchr(p1, '\n');
        if (p) {
            *p = '\0';
        }
        char *p2 = strchr(p1, '=');
        if (p2) {
            *p2 = '\0';
            p2++;
        }
        alltrim(p1);
        alltrim(p2);

        if (strcasecmp(param[i], "dpt_delay") == 0) {
            m_cfg.dpt_delay = strtol(p2, NULL, 10);
            if (m_cfg.dpt_delay < 0) {
                m_cfg.dpt_delay = 0;
            }
        } else if (strcasecmp(param[i], "swap_ch12") == 0) {
            if (strcasecmp(p2, "yes") == 0) {
                m_cfg.swap_channels12 = 1;
            } else if (strcasecmp(p2, "no") == 0) {
                m_cfg.swap_channels12 = 0;
            } else {
                m_cfg.swap_channels12 = strtol(p2, NULL, 10);
                if (m_cfg.swap_channels12 > 1) {
                    m_cfg.swap_channels12 = 1;
                }
            }
        }

        if (!p) {
            break;
        }
        p0 = p + 1;
    }

    return OK;
}

//---------------------------------------------------------
void cfg_get_app_path(int ac, char *av[])
{
    char *p = m_cfg.args;
    int ln = 0;
    int remain = (int) sizeof(m_cfg.args) - 1;
    for (int i = 1; (i < ac) && (remain > 0); i++) {
        ln = snprintf(p, remain, "%s ", av[i]);
        p += ln;
        remain -= ln;
    }

    char buf[512];
    strncpy(buf, av[0], sizeof(buf) - 1);
    DBGF(1, "cmd: %s %s", av[0], m_cfg.args);
    if (av[0][0] != '/') {
        ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
        if (len != -1) {
            buf[len] = '\0';
        }
    }

    //strncpy(buf, av[0], sizeof(buf) - 1);
    strncpy(m_cfg.appdir, dirname(buf), sizeof(m_cfg.appdir) - 2);
    DBGF(1, "dir: %s", m_cfg.appdir);

    strncpy(buf, av[0], sizeof(buf) - 1);
    strncpy(m_cfg.appname, basename(buf), sizeof(m_cfg.appname) - 1);
    p = strrchr(m_cfg.appname, '.');
    if (p) {
        *p = '\0';
    }
    DBGF(1, "name: %s", m_cfg.appname);
    //DBGF(1, "app: %s/%s", m_cfg.appdir, m_cfg.appname);

    //if (chdir(m_cfg.appdir) == -1) {
    //    DBGF(1, "can't change folder to %s (%s)", m_cfg.logfname, strerror(errno));
    //}
    p = getcwd(buf, sizeof(buf));
    DBGF(1, "work dir: %s", p);
}

//---------------------------------------------------------
bool cfg_read()
{
    struct stat st;
    static char cfgfilename[1024] = {0};

    char *p = getenv("LANG");
    if (p) {
        strncpy(m_cfg.locale, p, sizeof(m_cfg.locale) - 1);
    }

    //---------------------------------
    if (m_cfg.mode == MODE_UNKN) {
        snprintf(cfgfilename, sizeof(cfgfilename), "%s/.ATLAS", m_cfg.appdir);
        DBGF(1, "check %s", cfgfilename);
        if (stat(cfgfilename, &st) == 0) {
            m_cfg.mode = MODE_ATLAS;
        } else {
            snprintf(cfgfilename, sizeof(cfgfilename), "%s/.APS", m_cfg.appdir);
            DBGF(1, "check %s", cfgfilename);
            if (stat(cfgfilename, &st) == 0) {
                m_cfg.mode = MODE_APS;
            }
        }
        if (m_cfg.mode == MODE_UNKN) {
            LOG(ERROR, "unknown work mode - %d", (int) m_cfg.mode);
            DBGF(0, "config file not found %s", cfgfilename);
            DBGF(0, "unknown work mode - %d\n", (int) m_cfg.mode);
            return false;
        }
    }

    //---------------------------------
    snprintf(cfgfilename, sizeof(cfgfilename), "%s/atlas.cfg", m_cfg.appdir);
    if (m_cfg.mode == MODE_APS) {
        snprintf(cfgfilename, sizeof(cfgfilename), "%s/siu.cfg", m_cfg.appdir);
    }
    DBGF(1, "config file: %s", cfgfilename);
    cfg_parse_file(cfgfilename);
    cfg_read_serial_number(m_cfg.mode);

    snprintf(m_cfg.siu_ini_fname, sizeof(m_cfg.siu_ini_fname), "%s/data.sts", m_cfg.appdir);
    snprintf(m_cfg.esz7_cfg_fname, sizeof(m_cfg.esz7_cfg_fname), "%s/esz7.sts", m_cfg.appdir);
    return OK;
}
