/*
 * A header file collecting together all generic details related to ARCP. 
 * Information in this file defines the low-level details of the protocol.
 */

#ifndef _ARCP_H
#define _ARCP_H

/* ======================================================================== */

/* For various optimisation reasons the socket API provided by NutOS is
 * different from the standard socket API available on most other platforms.
 * In particular, the socket type under NutOS is TCPSOCKET * (not int) and
 * recv() / send() aren't available (NutTcpReceive() and NutTcpSend() are
 * provided instead).  Include requirements are a bit different too.
 *
 * If the ARCP_NUTOS define is active, assume this is being compiled for
 * NutOS and make adjustments accordingly.
 *
 * Furthermore, under win32 (aka Winsock) things are different again, so try
 * to make allowances for that here too if the ARCP_WIN32 define is defined.
 */
#ifdef ARCP_NUTOS
  #include <sys/socket.h>
  #include <errno.h>
  #include <netinet/in.h>           /* For htonl() etc in arcp.c */
  #include <stdint.h>
  typedef TCPSOCKET *arcp_socket_t;
  #define arcp_socket_read(_fd,_buf,_len,_flags) NutTcpReceive(_fd,_buf,_len)
  #define arcp_socket_write(_fd,_buf,_len,_flags) NutTcpSend(_fd,_buf,_len)
  #define SOCKET_ERRNO(_sock) NutTcpError(_sock)
#else
  #ifdef ARCP_WIN32
    /* Borland C++ Builder needs this before including Winsock2.h */
    #include <windows.h>
    #include <winsock2.h>
    typedef SOCKET arcp_socket_t;
    #define arcp_socket_read recv
    /* Windoze API uses char* instead of void* as the buffer pointer,
     * so insert an explicit cast to quieten the warnings when a non-char*
     * pointer is passed.
     */
    #define arcp_socket_write(_fd,_buf,_len,_flags) send(_fd,(char *)(_buf),_len,_flags)
    #define SOCKET_ERRNO(_sock) WSAGetLastError()
    #ifndef EINTR
    #define EINTR WSAEINTR
    #define EWOULDBLOCK WSAEWOULDBLOCK
    /* Some (all?) windoze environments don't have MSG_NOSIGNAL */
    #ifndef MSG_NOSIGNAL
      #define MSG_NOSIGNAL 0
    #endif
    #endif
  #else
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <errno.h>
    #include <netinet/in.h>           /* For htonl() etc in arcp.c */
    #include <stdint.h>
    typedef signed int arcp_socket_t;
    #define arcp_socket_read recv
    #define arcp_socket_write send
    #define SOCKET_ERRNO(_sock) errno
  #endif
#endif

/* ARCP version number: 8 bits for major number, 8 bits for minor number */
#define ARCP_VERSION_MAJOR       0x01
#define ARCP_VERSION_MINOR       0x01

/* Macros to form the version number as sent through the ARCP */
#define ARCP_VERSION_WORD(_major,_minor) (((_major)<<8)|(_minor))

/* Defines to identify specific ARCP versions */
#define ARCP_VERSION_1_0         ARCP_VERSION_WORD(1,0)
#define ARCP_VERSION_1_1         ARCP_VERSION_WORD(1,1)


/* ARCP library version number.  This comprises a major, minor and build
 * triple.  Major/minor together describe the ARCP protocol version
 * implemented by the library while build allows for bugfixes and other
 * alterations which do not by themselves constitute an increase in the ARCP
 * protocol number.
 */
#define LIBARCP_VERSION_MAJOR    ARCP_VERSION_MAJOR
#define LIBARCP_VERSION_MINOR    ARCP_VERSION_MINOR
#define LIBARCP_VERSION_BUILD    0

#define LIBARCP_VERSION_WORD(_major,_minor,_build) ((((uint32)_major)<<16)|((_minor)<<8)|(_build))

/* A stringified version of the library version number, used by
 * libarcp-config to report a version number.  Ensure this is kept up to
 * date.
 */
#define LIBARCP_VERSION_STR      "1.1.1"

/* ======================================================================== */
/* A radar network has a 16 bit address space, divided into blocks/regions.
 * Each region is set aside for a particlar module class.
 */
#define ARCP_CLASS_SYSPC         0x0001   /* System PCs */
#define ARCP_CLASS_TM            0x0002   /* Transmit modules */
#define ARCP_CLASS_TM_PS         0x0003   /* TM power supplies */
#define ARCP_CLASS_RF_ROUTER     0x0004   /* RF router - combiner, beamsteering etc */
#define ARCP_CLASS_SUPPORT_MOD   0x0100   /* General support module */

/* Base addresses and masks for each of the defined classes */
#define ARCP_SYSPC_ADDR_BASE     0x0100
#define ARCP_SYSPC_ADDR_MASK     0x000f
#define ARCP_TM_ADDR_BASE        0x1000
#define ARCP_TM_ADDR_MASK        0x00ff
#define ARCP_TM_PS_ADDR_BASE     0x1200
#define ARCP_TM_PS_ADDR_MASK     0x00ff
#define ARCP_TM_ROUTER_ADDR_BASE 0x1400
#define ARCP_TM_ROUTER_ADDR_MASK 0x00ff
#define ARCP_SUPPORT_MOD_BASE    0x2000
#define ARCP_SUPPORT_MOD_MASK    0x001f

/* The lower 2 bytes of an IP number on a Radar Network is determined by the
 * ARCP address.  The upper 2 bytes are fixed at 172.16 (0xac10).  ORing
 * the ARCP Radar Network (RN) base with an ARCP module address gives a
 * full 32 bit IPv4 address.
 */
#define ARCP_RN_BASE    0xac100000L
#define ARCP_RN_MASK    0xffff0000L

/* The MAC address for a given module can be formed in the locally
 * administered range 42:54:52:44:XX:YY, where 0xXXYY is the ARCP module
 * address.
 */
#define ARCP_MAC_BASE   "\x42\x54\x52\x44\x00\x00"

/* ARCP-enabled modules listen on a particular TCP port for incoming 
 * connections 
 */
#define ARCP_TCP_PORT 49490

/* Maximum size of an ARCP message, in bytes.  This currently cannot
 * exceed 65536 bytes without a revision to the protocol.
 */
#define ARCP_MSG_MAX_SIZE       1024

/* Maximum size of a pulse sequence (ie: maximum number of entries).  The
 * setting of ARCP_MSG_MAX_SIZE will affect this to a certain extent, as
 * will the size of a single entry in the pulse sequence list.  This should
 * not exceed UINT_MAX/sizeof(arcp_pulseseq_entry_t) without special
 * attention in arcp_pulseseq_set_length().
 */
#define ARCP_MAX_PULSESEQ_SIZE   500

/* The maximum size of a pulse code, in bits.  This should not be changed
 * without verifying that the new size will fit in a "set pulse parameter"
 * packet of size ARCP_MSG_MAX_SIZE bytes and that it is less than 65536. 
 * Furthermore, since the pulse code is stored in an array of bytes one
 * might as well ensure the size is a multiple of 8.
 */
#define ARCP_MAX_PULSECODE_SIZE  512

/* Define some maximum limits for certain system parameters.  These do not
 * necessarily represent protocol limits; rather they are used to sanity
 * check incoming message streams and contain reasonable limits based on the
 * current hardware.
 */
/* Maximum number of chassis fans expected */
#define ARCP_MAX_N_CHASSIS_FANS         8
/* Maximum number of RF cards expected */
#define ARCP_MAX_N_RF_CARDS             9
/* Maximum number of outputs an RF card is expected to have */
#define ARCP_MAX_N_RF_CARD_OUTPUT       8
/* Maximum number of entries in a pulse sequence list */
#define ARCP_MAX_PULSE_SEQ_LENGTH    1024
/* Maximum number of STX2 units possible on a single controller card */
#define ARCP_STX2_MAX_N_STX2_UNITS     10
/* Maximum number of temperatures in a STX2 external unit status structure */
#define ARCP_STX2_EXTCOMB_MAX_N_TEMPERATURES 4
/* Maximum number of outputs on an external unit monitored by a STX2 */
#define ARCP_STX2_EXTCOMB_MAX_N_OUTPUTS      6

/* Maximum number of temperatures sensed by a BSM */
#define ARCP_BSM_MAX_N_TEMPERATURES          8

/* The maximum number of phases allowed in a single phase table.  This is
 * used strictly for sanity checking of input only and can be increased up
 * to 0xffff if necessary.
 */
#define ARCP_BSM_MAX_N_PHASES              32

/* Defines for the STX2 card map */
#define ARCP_STX2_CARDMAP_CONTROLLER    0x0001
#define ARCP_STX2_CARDMAP_RFDRIVER      0x0002
#define ARCP_STX2_CARDMAP_PA0           0x0004
#define ARCP_STX2_CARDMAP_PA1           0x0008
#define ARCP_STX2_CARDMAP_PA2           0x0010
#define ARCP_STX2_CARDMAP_PA3           0x0020
#define ARCP_STX2_CARDMAP_PA4           0x0040
#define ARCP_STX2_CARDMAP_PA5           0x0080
#define ARCP_STX2_CARDMAP_INT0          0x0100
#define ARCP_STX2_CARDMAP_INT1          0x0200
#define ARCP_STX2_CARDMAP_INT2          0x0400
#define ARCP_STX2_CARDMAP_INT3          0x0800
#define ARCP_STX2_CARDMAP_EXT0          0x1000
#define ARCP_STX2_CARDMAP_EXT1          0x2000
#define ARCP_STX2_CARDMAP_EXT2          0x4000
#define ARCP_STX2_CARDMAP_EXT3          0x8000

#define ARCP_STX2_CARDMAP_INT_MASK      (ARCP_STX2_CARDMAP_INT0|ARCP_STX2_CARDMAP_INT1|ARCP_STX2_CARDMAP_INT2|ARCP_STX2_CARDMAP_INT3)
#define ARCP_STX2_CARDMAP_EXT_MASK      (ARCP_STX2_CARDMAP_EXT0|ARCP_STX2_CARDMAP_EXT1|ARCP_STX2_CARDMAP_EXT2|ARCP_STX2_CARDMAP_EXT3)

/* ======================================================================== */

/* Define some types with specific sizes.  The following should be right
 * for compilation under gcc on 32-bit Linux and the AVR cross compiler.
 * It will probably work OK on 62-bit Linux systems as well, but this has
 * not been verified.  Essentially we use the C99 types if the compiler
 * supports them, or fall back to a "best guess" if it doesn't.
 */

#ifdef uint8_t
  typedef uint8_t            uint8;
  typedef int8_t             int8;
  typedef uint16_t           uint16;
  typedef int16_t            int16;
  typedef uint32_t           uint32;
  typedef int32_t            int32;
#else
  typedef unsigned char      uint8;
  typedef signed char        int8;
  typedef unsigned short int uint16;
  typedef signed short int   int16;
  typedef unsigned long int  uint32;
  typedef signed long int    int32;
#endif

/* ======================================================================== */

/* The ARCP magic number, found in the first 4 bytes of all ARCP packets */
#define ARCP_MAGIC_NUMBER              0x41524350UL

/* Message types */
enum {
  ARCP_MSG_COMMAND    = 0,
  ARCP_MSG_RESPONSE   = 1,
};
typedef uint8 arcp_msgtype_t;

/* Module types */
enum {
  ARCP_MODULE_ANY       = -1,
  ARCP_MODULE_NONE      = -1,
  ARCP_MODULE_MASTER    =  0,
  ARCP_MODULE_STX2      =  1,  /* Vhf Transmitter Module */
  ARCP_MODULE_BSM       =  2,  /* BeamSteering Module */
};
typedef signed char arcp_moduletype_t;

/* Command message IDs.
 * Note that commands less than 0 are error return values and should
 * not appear in the list of known commands.
 * The commands are arranged as follows:
 *   < 0    = error return values from selected functions
 *   0x0000 - 0x00ff = module-independent commands
 *   0x0100 - 0x01ff = commands for transmitter modules
 *   0x0200 - 0x023f = beamsteering module commands
 *   0x0240 - 0xffff = reserved
 */
enum {
  ARCP_ERROR_NOT_CMD          = -128,  /* Cmd expected but not received */
  ARCP_ERROR_UNKNOWN_CMD      = -127,  /* Unknown command flag */
  ARCP_CMD_RESET              = 0x0000,
  ARCP_CMD_PING               = 0x0001,
  ARCP_CMD_GET_SYSID          = 0x0002,
  ARCP_CMD_GET_SYSSTAT        = 0x0010,
  ARCP_CMD_SET_MODULE_ENABLE  = 0x0020,
  ARCP_CMD_SET_PULSE_PARAM    = 0x0101,
  ARCP_CMD_SET_PULSE_SEQ      = 0x0102,
  ARCP_CMD_SET_PULSE_SEQ_IDX  = 0x0103,
  ARCP_CMD_SET_TRIG_PARAM     = 0x0110,
  ARCP_CMD_SET_USRCTL_ENABLE  = 0x01f0,

  ARCP_CMD_SET_PHASE          = 0x0200,
};
typedef int16 arcp_cmd_id_t;

/* Response IDs and ARCP error codes.
 * Note that response IDs less than ARCP_RESP are error codes and should not
 * appear in the list of known responses.  Values between -128 and -3
 * inclusive are protocol-level errors.  Values less than -128 are available
 * for errors specific to modules/commands.
 */
enum {
  ARCP_ERROR_INTERNAL         = -128,  /* Internal error: fix the code! */
  ARCP_ERROR_LOCAL            = -127,  /* Local processing error (low mem) */
  ARCP_ERROR_SEQUENCE         = -126,  /* Protocol sequence error */
  ARCP_ERROR_BADMSG           = -125,  /* Message corrupt */
  ARCP_ERROR_BAD_PROTO_VER    = -124,  /* Protocol version unknown */
  ARCP_ERROR_BAD_RESPONSE     = -123,  /* Unexpected response received */
  ARCP_ERROR_CONN_TIMEOUT     = -122,  /* TCP connection send/recv timeout */
  ARCP_ERROR_CONN_DROPPED     = -121,  /* TCP connection closed */
  ARCP_ERROR_UNKNOWN_RESP     = -120,  /* Response ID unknown */
  ARCP_ERROR_NOT_RESP         = -119,  /* Response expected but not received */
  ARCP_RESP                   = -2,    /* Start of ARCP response codes */
  ARCP_RESP_UNK               = -2,    /*   Unknown message response */
  ARCP_RESP_NAK               = -1,    /*   Command NAKed */
  ARCP_RESP_ACK               = 0,     /*   Command ACKed */
  ARCP_RESP_SYSID             = 0x02,  /*   System ID response */
  ARCP_RESP_SYSSTAT           = 0x10,  /*   System status response */
};
typedef int16 arcp_resp_id_t;

/* Pulse types */
enum {
  ARCP_PULSE_SHAPE_NONE      = 0x00,        /* Empty pulse */
  ARCP_PULSE_SHAPE_EMPTY     = 0x00,
  ARCP_PULSE_SHAPE_SQUARE    = 0x01,        /* Square pulse */ 
  ARCP_PULSE_SHAPE_GAUSSIAN  = 0x02,        /* Gaussian pulse */
};
typedef int8 arcp_pulseshape_t;

/* Pulse progamming options */
#define ARCP_PULSE_NORMAL              0x0000
#define ARCP_PULSE_CONST_INTERBIT      0x0001
#define ARCP_PULSE_6dBFS_CUTOFF        0x0002

/* Trigger sources */
enum {
  ARCP_TRIG_SRC_EXT          = 0x00,
  ARCP_TRIG_SRC_INT          = 0x01,
};

/* External trigger options */
enum {
  ARCP_EXT_TRIG_OPT_NORMAL   = 0x00,
  ARCP_EXT_TRIG_OPT_INVERT   = 0x01,
  ARCP_EXT_TRIG_OPT_IS_GATE  = 0x02,

  ARCP_EXT_TRIG_OPT_MASK     = (ARCP_EXT_TRIG_OPT_INVERT|ARCP_EXT_TRIG_OPT_IS_GATE),
};

/* Pulse transmission flags (used in pulse sequence flags field) */
#define ARCP_PULSE_FLAG_NORMAL         0x0000
#define ARCP_PULSE_FLAG_INV            0x0001

/* ======================================================================== */
/* Support structures */

/* A type to use when identifying an ARCP connection to the library.
 * Providing this level of abstraction makes it much easier to maintain
 * multiple simultaneously open stateful ARCP connections in a single 
 * application.
 */
typedef struct arcp_handle_t {
  arcp_socket_t fd;
  uint16 connection_arcp_version;
} arcp_handle_t;

/* A type to support pulse codes of arbitary length */
typedef struct arcp_pulsecode_t {
  uint16 size, code_length;
  uint8  *data;
} arcp_pulsecode_t;

/* A structure to hold entries in a pulse sequence */
typedef struct arcp_pulseseq_entry_t {
  uint8 slot;
  uint8 flags;
} arcp_pulseseq_entry_t;

/* A structure to hold a phase entry for a beamsteering unit */
typedef struct arcp_phase_entry_t {
  uint16 channel;
  float phase;
} arcp_phase_entry_t;

/* ======================================================================== */
/* Structures used to group related data together for various processes.
 * The end user of this library will usually be using structures in this
 * group.
 */

/* Pulse parameters */
typedef struct arcp_pulse_t {
  arcp_pulseshape_t pulse_shape;
  uint16 pulse_ampl;
  uint16 pulse_options;
  uint32 pulse_width_ns;
  arcp_pulsecode_t *code;
} arcp_pulse_t;

/* A pulse sequence */
typedef struct arcp_pulseseq_t {
  uint16 length;
  arcp_pulseseq_entry_t *seq;
} arcp_pulseseq_t;

/* Trigger parameters */
typedef struct arcp_trigger_t {
  uint8 trigger_source;
  uint8 ext_trigger_options;
  uint16 int_trigger_freq;  
  uint16 pulse_predelay;    
} arcp_trigger_t;

/* System ID data */
typedef struct arcp_sysid_t {
  arcp_moduletype_t module_type;
  uint16 module_version;
  uint16 firmware_version;
  uint16 ctrl_board_logic_version;
  union {
    struct {
      uint16 card_map;
      uint32 pulse_slot_length;  /* In nanoseconds */
    } stx2;
    struct {
      uint16 channel_map;
    } bsm;
  } data;
} arcp_sysid_t;

/* Status of an RF output on an RF card */
typedef struct arcp_rf_card_output_stat_t {
  uint16 forward_power;  /* In Watts */
  int16 return_loss;
} arcp_rf_card_output_stat_t;

/* Status of an RF card */
typedef struct arcp_rf_card_stat_t {
  uint16 rail_supply;    /* Supply rail in mV */
  int16  heatsink_temp;  
  uint8  n_rf_outputs;   
  arcp_rf_card_output_stat_t *output_stat;
} arcp_rf_card_stat_t;

/* Status of an internal card.  Every internal card status structure should
 * contain at least these two fields.  Currently there are no internal card
 * types defined, so this is more or less a place-holder.
 */
typedef struct arcp_intcard_stat_t {
  uint8 flags;
  uint8 card_type;
} arcp_intcard_stat_t;

/* Status of an RF output on the external combiner.  At least for now
 * this is exactly the same as for an RF card output.
 */
typedef arcp_rf_card_output_stat_t arcp_extcomb_output_stat_t;

/* Status of an external combiner / TR-switch unit */
typedef struct arcp_extcomb_stat_t {
  uint8 flags;
  uint8 unit_type;
  uint8 n_temperatures;
  int8  temperature[ARCP_STX2_EXTCOMB_MAX_N_TEMPERATURES];
  uint8 n_outputs;
  arcp_extcomb_output_stat_t output[ARCP_STX2_EXTCOMB_MAX_N_OUTPUTS];
} arcp_extcomb_stat_t;

/* Status of a generic external unit.  Every unit status structure should
 * contain at least these two fields.
 */
typedef struct arcp_extunit_stat_t {
  uint8 flags;
  uint8 type;
} arcp_extunit_stat_t;

/* Union used to assimilate STX2 unit status structures */
typedef union arcp_stx2unit_union_t {
  arcp_extunit_stat_t unit;
  arcp_extcomb_stat_t comb;
} arcp_stx2unit_union_t;

/* Status data specific to a STX2 */
typedef struct arcp_stx2stat_t {
  uint16 status_code;
  uint8 chassis_datasize;
  uint16 rail_supply;    /* Supply rail in mV */
  uint16 rail_aux;       /* Auxillary power rail in mV */
  int8  ambient_temp;
  uint8 n_chassis_fans;
  uint16 *fan_speed;   
  uint16 card_map;     
  uint8 n_rf_cards;    
  arcp_rf_card_stat_t *rf_card_stat;
  uint8 n_units;
  arcp_stx2unit_union_t *unit_stat;
} arcp_stx2stat_t;

/* Status data specific to a BSM (beam steering module) */
typedef struct arcp_bsmstat_t {
  uint16 status_code;
  uint16 rail_supply;    /* Supply rail in mV */
  uint16 rail_aux;       /* Auxillary power rail in mV */
  int8  ambient_temp;
  uint16 channel_map;
  uint8 n_fans;
  uint16 fan_speed[ARCP_MAX_N_CHASSIS_FANS];
  uint8 n_heatsink_temps;
  int8  heatsink_temp[ARCP_BSM_MAX_N_TEMPERATURES];
} arcp_bsmstat_t;

/* Status of an ARCP node */
typedef struct arcp_sysstat_t {
  arcp_moduletype_t module_type;
  int8 module_status;
  union {
    void *data;
    arcp_stx2stat_t *stx2;
    arcp_bsmstat_t *bsm;
  } data; 
} arcp_sysstat_t;

/* ======================================================================== */
/* Structures used to manipulate ARCP messages on the wire.  These are
 * basically low-level structures and it is not expected that the end user
 * will need to manipulate any of these directly.
 */

typedef struct arcp_msg_header_t {
  uint32 magic_num;
  uint16 msg_length;
  uint16 exchange_id;
  arcp_msgtype_t msg_type;
  uint16 protocol_version;
} arcp_msg_header_t;

typedef struct arcp_msg_cmd_t {
  arcp_msg_header_t header;
  arcp_cmd_id_t id;
} arcp_msg_cmd_t;

typedef struct arcp_msg_cmd_enable_t {
  arcp_msg_header_t header;
  arcp_cmd_id_t cmd_id;
  int8 enable;
} arcp_msg_cmd_enable_t;

typedef struct arcp_msg_cmd_setpulse_t {
  arcp_msg_header_t header;
  arcp_cmd_id_t cmd_id;
  uint8 pulse_map_index;
  arcp_pulse_t pulse_param;
} arcp_msg_cmd_setpulse_t;

typedef struct arcp_msg_cmd_setseq_t {
  arcp_msg_header_t header;
  arcp_cmd_id_t cmd_id;
  arcp_pulseseq_t *seq;
} arcp_msg_cmd_setseq_t;

typedef struct arcp_msg_cmd_setseq_idx_t {
  arcp_msg_header_t header;
  arcp_cmd_id_t cmd_id;
  uint16 seq_index;
} arcp_msg_cmd_setseq_idx_t;

typedef struct arcp_msg_cmd_settrig_t {
  arcp_msg_header_t header;
  arcp_cmd_id_t cmd_id;
  arcp_trigger_t trig_param;
} arcp_msg_cmd_settrig_t;

typedef struct arcp_msg_cmd_usrctl_enable_t {
  arcp_msg_header_t header;
  arcp_cmd_id_t cmd_id;
  int8 enable;
} arcp_msg_cmd_usrctl_enable_t;

typedef struct arcp_msg_cmd_set_phase_t {
  arcp_msg_header_t header;
  arcp_cmd_id_t cmd_id;
  uint16 phase_slot;
  uint16 n_phases;
  arcp_phase_entry_t *phases;
} arcp_msg_cmd_set_phase_t;

typedef struct arcp_msg_resp_t {
  arcp_msg_header_t header;    
  arcp_resp_id_t id;
  int16 info_code;
} arcp_msg_resp_t;

typedef struct arcp_msg_resp_sysid_t {
  arcp_msg_header_t header;       
  arcp_resp_id_t resp_id;     
  int16 info_code;
  arcp_sysid_t *sysid;
} arcp_msg_resp_sysid_t;

typedef struct arcp_msg_resp_sysstat_t {
  arcp_msg_header_t header;
  arcp_resp_id_t resp_id;
  int16 info_code;
  arcp_sysstat_t *sysstat;
} arcp_msg_resp_sysstat_t;

typedef union arcp_msg_t {
  arcp_msg_header_t header;
  arcp_msg_cmd_t command;
  arcp_msg_cmd_enable_t cmd_enable;
  arcp_msg_cmd_setpulse_t cmd_set_pulse_param;
  arcp_msg_cmd_setseq_t cmd_set_pulse_seq;
  arcp_msg_cmd_setseq_idx_t cmd_set_pulse_seq_idx;
  arcp_msg_cmd_settrig_t cmd_set_trig_param;
  arcp_msg_cmd_usrctl_enable_t cmd_usrctl_enable;
  arcp_msg_cmd_set_phase_t cmd_set_phase;
  arcp_msg_resp_t response;
  arcp_msg_resp_sysid_t resp_sysid;
  arcp_msg_resp_sysstat_t resp_sysstat;
} arcp_msg_t;

/* A type to hold the actual ARCP message stream to be sent to (or read
 * from) an ARCP-enabled module.  Users of this code should not access the
 * fields of this type directly.
 */
typedef struct arcp_stream_t {
  uint16 size;
  uint8  *data, *head, *end;
  uint8  err;
} arcp_stream_t;

/* Some predefined sizes */
#define ARCP_HEADER_SIZE          11    /* In bytes */
#define ARCP_MAX_PULSECODE_SIZE  512    /* In bits */

/* ======================================================================== */
/* Module-specific defines */

/* NAK Error codes returned by selected ARCP modules.  The error codes
 * should fall in the range from -129 to -32768 inclusive.  Values in the
 * range from -200 to -300 are currently reserved for use by STX2s.
 */
enum {
  ARCP_STX2_ERROR_PULSE_TOO_LONG   = -200,  /* Req pulse too long for slot */
};

/* STX2 status code values */
enum {
  ARCP_STX2_STATUS_OK               = 0x0000,
  ARCP_STX2_STATUS_RF_DRV_OVERTEMP  = 0x0001,
  ARCP_STX2_STATUS_RF_PA_OVERTEMP   = 0x0002,
  ARCP_STX2_STATUS_EXTCOMB_OVERTEMP = 0x0004,
};

/* STX2 unit type flags for identifying data in the generic unit part of the
 * STX2 status structure.  Although the controller, RF driver and PAs are
 * allowed for here for completeness they are not generic units - they have
 * type-specific data files in status structures where appropriate.
 */
enum {
  ARCP_STX2_UNIT_NONE                  = 0x00,
  ARCP_STX2_UNIT_EXT_COMBINER_SPLITTER = 0x01,
  ARCP_STX2_UNIT_EXT_COMB_SPLIT_TRSW   = 0x01,
  ARCP_STX2_UNIT_CONTROLLER            = 0x02,
  ARCP_STX2_UNIT_RFDRV                 = 0x03,
  ARCP_STX2_UNIT_PA                    = 0x04,
  ARCP_STX2_UNIT_LASTTYPE              = 0x04,
};

/* BSM status code values */
enum {
  ARCP_BSM_STATUS_OK               = 0x0000,
  ARCP_BSM_STATUS_OVERTEMP         = 0x0001,
};

/* ======================================================================== */
/* Public functions */
#ifdef __cplusplus
extern "C" {
#endif

/* Functions to manipulate the pulse code type */
arcp_pulsecode_t *arcp_pulsecode_new(uint16 max_length);
void arcp_pulsecode_free(arcp_pulsecode_t *code);
uint8 *arcp_pulsecode_getdata(arcp_pulsecode_t *code);
uint16 arcp_pulsecode_getlength(arcp_pulsecode_t *code);
signed int arcp_pulsecode_setlength(arcp_pulsecode_t *code, uint16 new_length);
uint8  arcp_pulsecode_getbit(arcp_pulsecode_t *code, uint16 bitnum);
signed int arcp_pulsecode_setbit(arcp_pulsecode_t *code, uint16 bitnum, uint8 value);

/* Management of the pulse sequence type */
arcp_pulseseq_t *arcp_pulseseq_new(uint16 length);
void arcp_pulseseq_free(arcp_pulseseq_t *seq);
signed int arcp_pulseseq_set_length(arcp_pulseseq_t *seq, uint16 seq_length);
signed int arcp_pulseseq_set_entry(arcp_pulseseq_t *seq, uint16 entry, uint8 slot, uint8 flags);

/* Management of the STX2 status structure */
arcp_stx2stat_t *arcp_stx2stat_new(void);
void arcp_stx2stat_free(arcp_stx2stat_t *stat);
signed int arcp_stx2stat_set_n_chassis_fans(arcp_stx2stat_t *stat, uint8 n_chassis_fans);
signed int arcp_stx2stat_set_n_rf_cards(arcp_stx2stat_t *stat, uint8 n_rf_cards);
signed int arcp_stx2stat_set_n_rf_outputs(arcp_stx2stat_t *stat, uint8 card_index, uint8 n_rf_outputs);
signed int arcp_stx2stat_set_n_units(arcp_stx2stat_t *stat, uint8 n_stx2units);

/* Management of the BSM status structure */
arcp_bsmstat_t *arcp_bsmstat_new(void);
void arcp_bsmstat_free(arcp_bsmstat_t *stat);
signed int arcp_bsmstat_set_n_fans(arcp_bsmstat_t *stat, uint8 n_fans);

/* Management of the toplevel system id and system status structures */
arcp_sysid_t *arcp_sysid_new(void);
void arcp_sysid_free(arcp_sysid_t *sysid);
arcp_sysstat_t *arcp_sysstat_new(void);
void arcp_sysstat_free(arcp_sysstat_t *sysstat);
signed int arcp_sysstat_set_moduletype(arcp_sysstat_t *sysstat, arcp_moduletype_t type);

/* Management of the higher-level arcp_msg_t type which provides native
 * access to the raw content of messages.  Direct use of arcp_msg_t is
 * usually only needed in slave nodes.
 */
arcp_msg_t *arcp_msg_new(arcp_msgtype_t type);
arcp_cmd_id_t arcp_msg_get_cmd_id(arcp_msg_t *msg);
signed int arcp_msg_set_cmd_id(arcp_msg_t *msg, arcp_cmd_id_t id);
arcp_resp_id_t arcp_msg_get_resp_id(arcp_msg_t *msg);
signed int arcp_msg_set_resp_id(arcp_msg_t *msg, arcp_resp_id_t id);
uint16 arcp_msg_set_stream_size(arcp_msg_t *msg);
void arcp_msg_free(arcp_msg_t *msg);

/* Create and manage ARCP handles */
arcp_handle_t *arcp_handle_new(arcp_socket_t fd);
arcp_socket_t arcp_handle_get_socket(arcp_handle_t *handle);
uint16 arcp_handle_get_connection_arcp_version(arcp_handle_t *handle);
void arcp_handle_free(arcp_handle_t *handle);

/* Public functions to send, receive and verify ARCP messages.  Only the
 * read functions are likely to be used directly and then only by a
 * slave node.
 */
signed int arcp_ascii_or_arcp_read(arcp_handle_t *handle, arcp_msg_t **msg_read,
  unsigned char **ascii_read);
signed int arcp_ascii_read(arcp_handle_t *handle, unsigned char **ascii);
signed int arcp_msg_read(arcp_handle_t *handle, arcp_msg_t **msg_read);
signed int arcp_msg_write(arcp_handle_t *handle, arcp_msg_t *msg);
signed int arcp_check_resp_msg(arcp_msg_t *cmd, arcp_msg_t *resp);

/* Public functions to send the ARCP commands and deal with the response.
 * The reset, ping and get/set functions will usually be used by a master
 * while the send functions will normally only be needed by slave nodes.
 */
uint32 arcp_get_lib_version(void);
unsigned int arcp_get_lib_proto_version(void);
signed int arcp_reset(arcp_handle_t *handle);
signed int arcp_ping(arcp_handle_t *handle);
signed int arcp_get_sysid(arcp_handle_t *handle, arcp_sysid_t **sysid);
signed int arcp_get_sysstat(arcp_handle_t *handle, arcp_sysstat_t **sysstat);
signed int arcp_set_module_enable(arcp_handle_t *handle, uint8 enable);
signed int arcp_set_pulseparam(arcp_handle_t *handle, uint8 slot, arcp_pulse_t *param);
signed int arcp_set_pulseseq(arcp_handle_t *handle, arcp_pulseseq_t *seq);
signed int arcp_set_pulseseq_index(arcp_handle_t *handle, uint16 index);
signed int arcp_set_trigparam(arcp_handle_t *handle, arcp_trigger_t *param);
signed int arcp_set_usrctl_enable(arcp_handle_t *handle, uint8 enable);
signed int arcp_set_phase(arcp_handle_t *handle, uint16 phase_slot, arcp_phase_entry_t *phases, uint16 n_phases);
signed int arcp_send_ack(arcp_handle_t *handle, arcp_msg_t *cmd_msg);
signed int arcp_send_nak(arcp_handle_t *handle, arcp_msg_t *cmd_msg, int16 err_code);
signed int arcp_send_unk(arcp_handle_t *handle, arcp_msg_t *cmd_msg);
signed int arcp_send_sysid(arcp_handle_t *handle, arcp_msg_t *cmd_msg, arcp_sysid_t *sysid);
signed int arcp_send_sysstat(arcp_handle_t *handle, arcp_msg_t *cmd_msg, arcp_sysstat_t *sysstat);

#ifdef __cplusplus
}
#endif

/* ======================================================================== */

#endif
