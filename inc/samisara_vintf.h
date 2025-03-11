/*
 * samisara_vintf.h
 * 
 * Samisara vendor interface command protocol. Uses Feature Reports on a
 * vendor-specific interface. All fields in the report packets are
 * little endian.
 * 
 * Written & released by Keir Fraser <keir.xen@gmail.com>
 * 
 * This is free and unencumbered software released into the public domain.
 * See the file COPYING for more details, or visit <http://unlicense.org>.
 */

#define SAMISARA_VINTF_USAGE_PAGE 0xffc1
#define SAMISARA_VINTF_REPORT_ID  0x01
#define SAMISARA_VINTF_REPORT_SZ  48

/*
 * SAMISARA COMMAND SET
 * 
 * These are sent as Feature Reports:
 *  uint8_t command_id;
 *  uint8_t command_length;
 *  <...command-specific data...>
 *  uint8_t pad_mbz[];
 */

/* Which subreport is reported by GetFeatureReport. */
#define SAMISARA_CMD_SUBREPORT          0
struct packed samisara_cmd_subreport {
    uint16_t idx;
};

/* Reset into DFU mode. */
#define SAMISARA_CMD_DFU                1
struct packed samisara_cmd_dfu {
    uint32_t deadbeef;
};

#define SAMISARA_CMD_MAX                1

/*
 * SAMISARA SUBREPORTS
 * 
 * Specified by SAMISARA_CMD_SUBREPORT and retrieved via GetFeatureReport().
 * 
 * These are returned as:
 * uint8_t subreport_id;
 * uint8_t subreport_length;
 * <...subreport-specific data...>
 * uint8_t pad_mbz[];
 */

#define SAMISARA_SUBREPORT_INFO         0
struct packed samisara_subreport_info {
    uint16_t max_cmd;
    uint16_t max_subreport;
    uint16_t cmd_result; /* Result of last command: SAMISARA_RESULT_* */
};

/* Build-info strings. Report length does not include NUL termination. */
#define SAMISARA_SUBREPORT_BUILD_VER    1
#define SAMISARA_SUBREPORT_BUILD_DATE   2

#define SAMISARA_SUBREPORT_MAX          2

/*
 * COMMAND RESULTS
 */
#define SAMISARA_RESULT_OKAY            0
#define SAMISARA_RESULT_BAD_CMD         1

/*
 * Local variables:
 * mode: C
 * c-file-style: "Linux"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
