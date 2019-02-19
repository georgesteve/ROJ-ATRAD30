/*
 * A module implementing support functions related to the Atrad Radar
 * Control Protocol (ARCP).  The functions contained herein contain a vast
 * majority of the ARCP implementation; in theory, the only additional code
 * required to make a functional ARCP master or slave is some glue code.
 *
 * It is intended that this module is sufficiently portable to allow it to
 * be compilable for all architectures which deal with the ARCP.
 *
 * Note that in many places (for example, those dealing with conditional
 * expressions), seemingly unnecessary typecasts are applied.  These are
 * included to prevent warnings with certain brain-dead windoze compilers
 * (Borland C++ Builder for instance) which seem too stupid to preserve
 * types throughout the evaluation of the affected expressions.  For
 * example, affected compilers seem to promote the type of conditional
 * expressions to "int" (or something) so that when it's assigned to a
 * variable of the same (non-default "int") type as both the expressions a
 * truncation warning is produced.  Another example: assuming foo is of type
 * uint16, the expression
 *   foo += 4;
 * can sometimes produce a warning.  Go figure - that's windoze for you.
 */

#include <stdlib.h>
#include <string.h>
#include "arcp.h"

/* ======================================================================== */

/* The next exchange ID to use */
static unsigned short int exchange_id = 0;

/* Flags used internally by arcp_socket_process() */
#define MSG_ARCP             0x0001
#define MSG_ASCII            0x0002
#define MSG_ALL              (MSG_ARCP | MSG_ASCII)

/* ======================================================================== */

/* Used in arcp_pulsecode_new() */
signed int arcp_pulsecode_setsize(arcp_pulsecode_t *code, uint16 newsize);

/* ======================================================================== */

static void *set_new_size(char *buf, unsigned int new_size, 
  unsigned int old_size) {
/*
 * For internal use: allocates new_size bytes to the area pointed to by buf
 * and returns a pointer to the new storage location (which might be 
 * different to buf).
 *
 * If new_size is 0 the existing block (if any) will be freed and NULL will
 * be returned.
 *
 * Returns a pointer to the allocated storage on success, or NULL on error
 * or if new_size is 0.
 */
char *res;

  /* If there's no change in the size requested just exit */
  if (new_size == old_size)
    return buf;
    
  /* Allocate memory as required.  We can't just use realloc() here since
   * the avr compiler doesn't seem to like it for some reason. 
   */
  if (new_size == 0)
    res = NULL;
  else {
    res = malloc(new_size);
    if (res == NULL)
      return NULL;
  }

  if (buf!=NULL && res!=NULL) {
    /* Copy old data to new location */
    size_t n = new_size;
    if (new_size > old_size)
      n = old_size;
    if (n!=0)
      memcpy(res,buf,n);
    free(buf);
  }

  /* If the new block is longer than before, zero the new elements */
  if (new_size > old_size) {
    memset(res+old_size,0,new_size-old_size);
  }

  return res;
}
/* ======================================================================== */

arcp_pulsecode_t *arcp_pulsecode_new(uint16 size) {
/*
 * Creates an arcp_pulsecode_t object which sufficient storage to hold
 * size bits.
 */
arcp_pulsecode_t *res;

  if (size==0)
    return NULL;
  res = malloc(sizeof(arcp_pulsecode_t));
  if (res==NULL)
    return NULL;

  /* Allocate storage for the pulse code */
  res->size = 0;
  res->data = NULL;
  if (arcp_pulsecode_setsize(res, size) < 0) {
    free(res);
    return NULL;
  }
    
  res->code_length = 0;
  return res;
}
/* ======================================================================== */

void arcp_pulsecode_free(arcp_pulsecode_t *code) {
/*
 * Deallocates memory used by the given pulse code object.
 */
  if (code == NULL)
    return;
  if (code->data != NULL)
    free(code->data);
  free(code);
}
/* ======================================================================== */

uint8 *arcp_pulsecode_getdata(arcp_pulsecode_t *code) {
/*
 * Returns a pointer to the actual data array used to store the pulse code.
 */
  return code->data;
}
/* ======================================================================== */

uint16 arcp_pulsecode_getsize(arcp_pulsecode_t *code) {
/*
 * Internal function: returns the current size of the pulse code object in
 * bits.
 */
  return code->size;
}
/* ======================================================================== */

signed int arcp_pulsecode_setsize(arcp_pulsecode_t *code, uint16 newsize) {
/*
 * Internal function: Changes the pulse code storage so it is sufficient to
 * hold newsize bits.  Returns 0 on success or -1 if an error occured.
 */
uint16 osize, nsize;
uint8 *buf;

  if (code->size == newsize)
    return 0;

  if (newsize > ARCP_MAX_PULSECODE_SIZE)
    return -1;

  /* Typecasts required to counter warnings in some silly windoze 
   * compilers which promote the type of the conditional expression for
   * some odd reason.
   */
  osize = (uint16)(code->size==0?0:1+(code->size-1)/8);
  nsize = (uint16)(newsize==0?0:1+(newsize-1)/8);
  buf = set_new_size((void *)code->data, nsize, osize);
  if (nsize!=0 && buf==NULL)
    return -1;

  code->data = buf;
  code->size = newsize;
  return 0;
}
/* ======================================================================== */

uint16 arcp_pulsecode_getlength(arcp_pulsecode_t *code) {
/*
 * Returns the length of the given pulse code in bits.  If code is NULL 
 * a length of 1 is returned since a NULL code implies a monopulse.
 */

  /* Typecast required to counter a warning in some silly windoze 
   * compilers which promote the type of the conditional expression for
   * some odd reason.
   */
  return (uint16)(code?code->code_length:1);
}
/* ======================================================================== */

signed int arcp_pulsecode_setlength(arcp_pulsecode_t *code, uint16 new_length) {
/*
 * Sets the pulse code length.  Note that this does not populate the pulse
 * code in any way - it simply sets the upper bound of significant bits. 
 * Returns -1 on error (new_length > size and memory exhausted), or 0 on
 * success.
 */
  if (code->size < new_length)
    if (arcp_pulsecode_setsize(code, new_length) < 0)
      return -1;

  code->code_length = new_length;
  return 0;
}
/* ======================================================================== */

uint8 arcp_pulsecode_getbit(arcp_pulsecode_t *code, uint16 bitnum) {
/*
 * Returns the value of the given bit in the supplied pulse code (0 or 1). 
 * If code is NULL or bitnum is out of range, 0 is returned.
 */
  /* Typecast required to counter a warning in some silly windoze 
   * compilers which promote the type of the conditional expression for
   * some odd reason.
   */
  return (uint8)((code && bitnum<code->code_length)?
    (code->data[bitnum/8] & (1<<(bitnum%8)))!=0:0);
}
/* ======================================================================== */

signed int arcp_pulsecode_setbit(arcp_pulsecode_t *code, uint16 bitnum, 
  uint8 value) {
/*
 * Sets the given bit to the requested value in the supplied pulse code.
 * The code's length is expanded to include the new bit if necessary.
 * If the requested bit number is beyond the code's maximum size the storage
 * space is extended to accommodate it.
 *
 * This function returns 0 on success or -1 if a memory allocation error
 * prevented the storage expansion necessary to store the requested bit.
 *
 * Note: the typecasts are required to counter warnings from some silly
 * windoze compilers which promote the type of the affected expressions for
 * some odd reason.
 */

  /* Extend storage space for the pulse code if necessary */
  if (bitnum >= code->size)
    if (arcp_pulsecode_setsize(code, (uint16)(bitnum+1)) < 0)
      return -1;

  if (value)
    code->data[bitnum/8] |= (uint8)(1<<(bitnum%8));
  else
    code->data[bitnum/8] &= (uint8)(~(1<<(bitnum%8)));
  if (bitnum >= code->code_length)
    code->code_length = (uint16)(bitnum+1);
  return 0;
}
/* ======================================================================== */

arcp_pulseseq_t *arcp_pulseseq_new(uint16 length) {
/*
 * Allocates a new pulse sequence object with sufficient storage to store
 * length elements.
 */
arcp_pulseseq_t *res = calloc(1,sizeof(arcp_pulseseq_t));

  if (res == NULL)
    return NULL;
  if (arcp_pulseseq_set_length(res, length) < 0) {
    free(res);
    return NULL;
  }
  return res;
}
/* ======================================================================== */

void arcp_pulseseq_free(arcp_pulseseq_t *seq) {
/*
 * Deallocates storage used by the given pulse sequence object.
 */
  if (seq==NULL)
    return;
  if (seq->seq != NULL)
    free(seq->seq);
  free(seq);
}
/* ======================================================================== */

signed int arcp_pulseseq_set_length(arcp_pulseseq_t *seq, uint16 seq_length) {
/*
 * Sets the length of the given sequence object.  If sequence data is
 * already present, as much of the data as possible is preserved.  Note that
 * seq_length is the number of entries in the sequence - not the number of
 * bytes taken to store the sequence.
 *
 * Returns 0 on success or -1 on failure.  Failure can occur if there's
 * insufficient memory to store the newly requested sequence (ARCP_ERROR_LOCAL)
 * or if too long a length was requested (ARCP_ERROR_INTERNAL).
 */
  /* Ensure the requested length is within valid limits */
arcp_pulseseq_entry_t *buf;

  if (seq_length > ARCP_MAX_PULSESEQ_SIZE)
    return ARCP_ERROR_INTERNAL;

  buf = set_new_size((void *)seq->seq,
          seq_length*sizeof(arcp_pulseseq_entry_t),
          seq->length*sizeof(arcp_pulseseq_entry_t));
  if (seq_length!=0 && buf==NULL)
    return ARCP_ERROR_LOCAL;
  
  /* Store the new sequence length */
  seq->seq = buf;
  seq->length = seq_length;

  return 0;
}
/* ======================================================================== */

signed int arcp_pulseseq_set_entry(arcp_pulseseq_t *seq, uint16 entry,
  uint8 slot, uint8 flags) {
/*
 * Sets the contents of the entry'th entry in seq to the slot/flags given.
 * Returns -1 if the given entry doesn't exist, or 0 on success.  Note that
 * entry and slot are zero-based.
 */
  if (entry >= seq->length)
    return -1;
  seq->seq[entry].slot = slot;
  seq->seq[entry].flags = flags;
  return 0;
}
/* ======================================================================== */

arcp_stx2stat_t *arcp_stx2stat_new(void) {
/*
 * Creates a new STX2 status object and returns a pointer to it.  If
 * allocation failed NULL is returned.
 */
  return calloc(1,sizeof(arcp_stx2stat_t));
}
/* ======================================================================== */

void arcp_stx2stat_free(arcp_stx2stat_t *stat) {
/*
 * Deallocates all storage used by the given STX2 status object.
 */
uint8 rf_card;

  if (stat == NULL)
    return;
  if (stat->fan_speed != NULL)
    free(stat->fan_speed);

  if (stat->rf_card_stat != NULL) {
    for (rf_card=0; rf_card<stat->n_rf_cards; rf_card++) {
      if (stat->rf_card_stat[rf_card].output_stat != NULL)
        free(stat->rf_card_stat[rf_card].output_stat);
    }
    free(stat->rf_card_stat);
  }

  if (stat->unit_stat != NULL) {
    free(stat->unit_stat);
  }
  free(stat);
}
/* ======================================================================== */

signed int arcp_stx2stat_set_n_chassis_fans(arcp_stx2stat_t *stat, 
  uint8 n_chassis_fans) {
/* 
 * Sets the number of chassis fans in the STX2 status object.  If chassis
 * fans have already been allocated, as much of the data is preserved as
 * possible.
 *
 * The chassis datasize field in the arcp_stx2stat_t structure is updated by
 * this function.
 *
 * Returns 0 on success or an ARCP_ERROR_* code on failure.  Failure can
 * occur if there's insufficient memory to store the newly requested list of
 * fan speeds (ARCP_ERROR_LOCAL), or if too many fans were requested
 * (ARCP_ERROR_INTERNAL).
 */
uint16 *buf;

  if (n_chassis_fans > ARCP_MAX_N_CHASSIS_FANS)
    return ARCP_ERROR_LOCAL;

  /* Each fanspeed record is 2 bytes */
  buf = set_new_size((void *)stat->fan_speed, 
          2*n_chassis_fans, 2*stat->n_chassis_fans);
  if (n_chassis_fans!=0 && buf==NULL)
    return ARCP_ERROR_LOCAL;

  /* Store the new number of chassis fans */
  stat->fan_speed = buf;
  stat->n_chassis_fans = n_chassis_fans;

  /* Also update the chassis datasize (in bytes) for completeness.  This is
   * used when encoding the message stream.  The typecast is required
   * for some silly windoze compilers which promote the expression's type
   * for no obvious reason.
   */
  stat->chassis_datasize = (uint8)(7 + 2*n_chassis_fans);

  return 0;
}
/* ======================================================================== */

signed int arcp_stx2stat_set_n_rf_cards(arcp_stx2stat_t *stat, uint8 n_rf_cards) {
/* 
 * Sets the number of RF cards represented in the given status object.  If space
 * for card data has already been allocated, as much data as possible is
 * preserved.
 *
 * Returns 0 on success or an ARCP_ERROR_* on failure.  Failure can occur if
 * there's insufficient memory to store the newly requested number of cards
 * (ARCP_ERROR_LOCAL).
 */
arcp_rf_card_stat_t *buf;

  buf = set_new_size((void *)stat->rf_card_stat,
          sizeof(arcp_rf_card_stat_t)*n_rf_cards, 
          sizeof(arcp_rf_card_stat_t)*stat->n_rf_cards);
  if (n_rf_cards!=0 && buf==NULL)
    return ARCP_ERROR_LOCAL;

  /* Store the new number of RF cards */
  stat->rf_card_stat = buf;
  stat->n_rf_cards = n_rf_cards;

  return 0;
}
/* ======================================================================== */

signed int arcp_stx2stat_set_n_rf_outputs(arcp_stx2stat_t *stat,
  uint8 card_index, uint8 n_rf_outputs) {
/* 
 * Sets the number of RF outputs on the given RF card in the supplied STX2
 * status object.  If space for RF output data has already been allocated,
 * as much of the data as possible is preserved.
 *
 * Returns 0 on success or an ARCP_ERROR_* code on failure.  Failure can
 * occur if there's insufficient memory to store the newly requested number
 * of RF outputs (ARCP_ERROR_LOCAL) or if card_index is out of range
 * (ARCP_ERROR_INTERNAL).
 */
arcp_rf_card_output_stat_t *buf;

  if (card_index >= stat->n_rf_cards)
    return ARCP_ERROR_INTERNAL;

  buf = set_new_size((void *)stat->rf_card_stat[card_index].output_stat,
          sizeof(arcp_rf_card_output_stat_t)*n_rf_outputs, 
          sizeof(arcp_rf_card_output_stat_t)*stat->rf_card_stat[card_index].n_rf_outputs);
  if (n_rf_outputs!=0 && buf==NULL)
    return ARCP_ERROR_LOCAL;

  /* Store the new number of RF outputs */
  stat->rf_card_stat[card_index].output_stat = buf;
  stat->rf_card_stat[card_index].n_rf_outputs = n_rf_outputs;

  return 0;
}
/* ======================================================================== */

signed int arcp_stx2stat_set_n_units(arcp_stx2stat_t *stat, uint8 n_units) {
/*
 * Sets the number of STX2 units in use within the given status structure. 
 * This involves creating the dynamically allocated unit status structure
 * and initialising them with sensible values.
 *
 * returns ARCP_ERROR_LOCAL on failure due to insufficient memory, or 0 on
 * success.
 */
arcp_stx2unit_union_t *buf;

  if (stat==NULL || n_units>ARCP_STX2_MAX_N_STX2_UNITS)
    return ARCP_ERROR_INTERNAL;

  buf = set_new_size((void *)stat->unit_stat,
          sizeof(arcp_stx2unit_union_t)*n_units,
          sizeof(arcp_stx2unit_union_t)*stat->n_units);
  if (n_units!=0 && buf==NULL)
    return ARCP_ERROR_LOCAL;

  stat->unit_stat = buf;
  stat->n_units = n_units;

  return 0;
}
/* ======================================================================== */

arcp_bsmstat_t *arcp_bsmstat_new(void) {
/*
 * Creates a new BSM status object and returns a pointer to it.  If
 * allocation failed NULL is returned.
 */
  return calloc(1,sizeof(arcp_bsmstat_t));
}
/* ======================================================================== */

void arcp_bsmstat_free(arcp_bsmstat_t *stat) {
/*
 * Deallocates all storage used by the given BSM status object.
 */
  free(stat);
}
/* ======================================================================== */

signed int arcp_bsmstat_set_n_fans(arcp_bsmstat_t *stat, uint8 n_fans) {
/* 
 * Sets the number of fans in the BSM status object.  If fans have already
 * been set, as much of the data is preserved as possible.
 *
 * Returns 0 on success or an ARCP_ERROR_* code on failure.  Failure can
 * occur if if too many fans were requested (ARCP_ERROR_INTERNAL).
 */
  if (n_fans > ARCP_MAX_N_CHASSIS_FANS)
    return ARCP_ERROR_LOCAL;

  /* Store the new number of chassis fans */
  stat->n_fans = n_fans;

  return 0;
}
/* ======================================================================== */

arcp_sysid_t *arcp_sysid_new(void) {
/*
 * Creates a new sysid object.  Currently this does not have to be a dynamic
 * object, but in future it might be useful to have this flexibility.
 */
  return calloc(1,sizeof(arcp_sysid_t));
}
/* ======================================================================== */

void arcp_sysid_free(arcp_sysid_t *sysid) {
/*
 * Deallocates memory used by the given sysid object.
 */
  if (sysid != NULL)
    free(sysid);
}
/* ======================================================================== */

arcp_sysstat_t *arcp_sysstat_new(void) {
/*
 * Creates a new system status object.  Currently this does not have to be a
 * dynamically sized object, but in future it might be useful to have this
 * flexibility.
 */
arcp_sysstat_t *res = calloc(1,sizeof(arcp_sysstat_t));

  if (res != NULL)
    res->module_type = -1;
  return res;
}
/* ======================================================================== */

void arcp_sysstat_free(arcp_sysstat_t *sysstat) {
/*
 * Deallocates memory used by the given system status object.
 */
  if (sysstat == NULL)
    return;
  switch (sysstat->module_type) {
    case ARCP_MODULE_STX2:
      arcp_stx2stat_free(sysstat->data.stx2);
      break;
    case ARCP_MODULE_BSM:
      arcp_bsmstat_free(sysstat->data.bsm);
      break;
  }
  free(sysstat);
}
/* ======================================================================== */

signed int arcp_sysstat_set_moduletype(arcp_sysstat_t *sysstat, arcp_moduletype_t type) {
/*
 * Sets the module type of the given status structure and configures any
 * associated dynamic structures.
 */
  if (sysstat->module_type >= 0)
    return ARCP_ERROR_INTERNAL;

  switch (type) {
    case ARCP_MODULE_STX2:
      if ((sysstat->data.stx2 = arcp_stx2stat_new()) == NULL)
        return ARCP_ERROR_LOCAL;
      break;
    case ARCP_MODULE_BSM:
      if ((sysstat->data.bsm = arcp_bsmstat_new()) == NULL)
        return ARCP_ERROR_LOCAL;
      break;
  }

  sysstat->module_type = type;
  return 0;
}
/* ======================================================================== */

static signed int arcp_id_is_response(signed int id) {
/*
 * Returns 1 if the given ID is an ARCP response code, or 0 if it represents
 * an error code.
 */
  return id >= ARCP_RESP;
}
/* ======================================================================== */

arcp_msg_t *arcp_msg_new(arcp_msgtype_t type) {
/*
 * Creates a new dynamically allocated arcp_msg_t object.  This object is
 * used to extract information from or put information into an ARCP stream
 * ready for transmission.  Using this intermediatory object removes the
 * need for a programmer to deal directly with the format of the ARCP stream
 * itself.
 *
 * NULL is returned if there is no memory available to satisfy the request
 * or an unknown message type was passed.  Otherwise a pointer to the newly
 * created object is returned.
 */
arcp_msg_t *msg;

  if (type!=ARCP_MSG_COMMAND && type!=ARCP_MSG_RESPONSE)
    return NULL;
  /* Create and zero the object */
  msg = calloc(1,sizeof(arcp_msg_t));
  if (msg == NULL)
    return NULL;

  /* Do some basic initialisation */
  msg->header.magic_num = ARCP_MAGIC_NUMBER;
  msg->header.msg_type = type;
  msg->header.protocol_version = 0;

  switch (type) {
    case ARCP_MSG_COMMAND:
      msg->command.id = -1;
      break;
    case ARCP_MSG_RESPONSE:
      msg->response.id = -1;
      break;
  }

  return msg;
}
/* ======================================================================== */

arcp_cmd_id_t arcp_msg_get_cmd_id(arcp_msg_t *msg) {
/*
 * Returns the command ID from the given ARCP message or an ARCP_ERROR_*
 * code if a problem was encountered.
 */
  /* Return ARCP_ERROR_NOT_CMD if the supplied message isn't a command */
  if (msg->header.msg_type != ARCP_MSG_COMMAND)
    return ARCP_ERROR_NOT_CMD;

  return msg->command.id;
}
/* ======================================================================== */

signed int arcp_msg_set_cmd_id(arcp_msg_t *msg, arcp_cmd_id_t id) {
/*
 * Sets the command ID of the given ARCP message.  Returns 0 on success. 
 * Error conditions are:
 *   - the supply of a message which isn't a command message
 *     (ARCP_ERROR_NOT_CMD will be returned)
 *   - an attempt to set the command ID of a message which already has
 *     had its command ID set (ARCP_ERROR_INTERNAL will be returned).
 */
  /* Return ARCP_ERROR_NOT_CMD if the supplied message isn't a command */
  if (msg->header.msg_type != ARCP_MSG_COMMAND)
    return ARCP_ERROR_NOT_CMD;

  /* If the command ID has already been set, return ARCP_ERROR_INTERNAL */
  if (msg->command.id != -1)
    return ARCP_ERROR_INTERNAL;

  msg->command.id = id;
  return 0;
}
/* ======================================================================== */

arcp_resp_id_t arcp_msg_get_resp_id(arcp_msg_t *msg) {
/*
 * Returns the response ID from the given ARCP message or an ARCP_ERROR_*
 * code if a problem was encountered.
 */
  /* Return ARCP_ERROR_NOT_RESP if the supplied message isn't a response */
  if (msg->header.msg_type != ARCP_MSG_RESPONSE)
    return ARCP_ERROR_NOT_RESP;

  return msg->response.id;
}
/* ======================================================================== */

signed int arcp_msg_set_resp_id(arcp_msg_t *msg, arcp_resp_id_t id) {
/*
 * Sets the given message's response ID to that given, and configures any
 * dynamic structures required to support this response type.
 */
  /* Return ARCP_ERROR_NOT_RESP if the supplied message isn't a response */
  if (msg->header.msg_type != ARCP_MSG_RESPONSE)
    return ARCP_ERROR_NOT_RESP;

  /* If the response ID has already been set, return ARCP_ERROR_INTERNAL */
  if (msg->response.id != -1)
    return ARCP_ERROR_INTERNAL;

  msg->response.id = id;
  return 0;
}
/* ======================================================================== */

uint16 arcp_msg_set_stream_size(arcp_msg_t *msg) {
/*
 * Calculates the size of the ARCP stream needed to send the given message
 * as it stands at the moment.  Note that while most messages are of fixed
 * length there are some which depend on dynamic factors such as the number
 * of chassis fans present.
 * 
 * Note also that the sizes for fixed-size messages are hardcoded.  sizeof()
 * isn't used because structure alignment issues will cause inaccuracies.
 * While this could be worked around for gcc using the 
 *   __attribute__((__packed__))
 * attribute this is not portable.
 *
 * Return value is the message size.  This is also stored in the msg_length
 * field of the message's header for convenience.
 *
 * Note: the explicit typecasts are required to counter a warning in some
 * silly windoze compilers which promote the types of the affected
 * expressions for some odd reason.
 */
uint16 idx, len;

  if (msg == NULL)
    return 0;
  switch (msg->header.msg_type) {
    case ARCP_MSG_COMMAND: {
      switch (msg->command.id) {
        case ARCP_CMD_SET_MODULE_ENABLE:
        case ARCP_CMD_SET_USRCTL_ENABLE:
          len = ARCP_HEADER_SIZE + 3;
          break;
        case ARCP_CMD_SET_PULSE_PARAM:
          len = ARCP_HEADER_SIZE +14;
          if (msg->cmd_set_pulse_param.pulse_param.code!=NULL &&
              arcp_pulsecode_getlength(msg->cmd_set_pulse_param.pulse_param.code)!=0) {
            len += (uint16)(1+((arcp_pulsecode_getlength(msg->cmd_set_pulse_param.pulse_param.code)-1)/8));
          }
          break;
        case ARCP_CMD_SET_PULSE_SEQ:
          len = (uint16)(ARCP_HEADER_SIZE + 2 + 2 + sizeof(arcp_pulseseq_entry_t)*
                  msg->cmd_set_pulse_seq.seq->length);
          break;
        case ARCP_CMD_SET_PULSE_SEQ_IDX:
          len = ARCP_HEADER_SIZE + 2 + 2;
          break;
        case ARCP_CMD_SET_TRIG_PARAM:
          len = ARCP_HEADER_SIZE + 2 + 6;
          break;

        case ARCP_CMD_SET_PHASE:
          len = ARCP_HEADER_SIZE + 2 + 2 + 2 + msg->cmd_set_phase.n_phases*6;
          break;

        default:
          /* All other commands comprise of only the header and command ID */
          len = ARCP_HEADER_SIZE + 2;
      }
      break;
    }
    case ARCP_MSG_RESPONSE: {
      switch (msg->response.id) {
        case ARCP_RESP_SYSID:
          len = ARCP_HEADER_SIZE + 11;   /* Base size of SYSID message */
          switch (msg->resp_sysid.sysid->module_type) {
            case ARCP_MODULE_STX2: len += (uint16)6; break;
            case ARCP_MODULE_BSM: len += (uint16)2; break;
          }
          break;
        case ARCP_RESP_SYSSTAT:
          len = ARCP_HEADER_SIZE + 6;    /* Base size of SYSSTAT message */
          switch (msg->resp_sysstat.sysstat->module_type) {
            case ARCP_MODULE_STX2:
              len += (uint16)(13 + 2*msg->resp_sysstat.sysstat->data.stx2->n_chassis_fans);
              for (idx=0; idx<msg->resp_sysstat.sysstat->data.stx2->n_rf_cards; idx++) {
                len += (uint16)(5 + 4*msg->resp_sysstat.sysstat->data.stx2->rf_card_stat[idx].n_rf_outputs);
              }
              if (msg->resp_sysstat.sysstat->data.stx2->n_units != 0) {
                len += (uint16)2 * msg->resp_sysstat.sysstat->data.stx2->n_units;
                for (idx=0; idx<msg->resp_sysstat.sysstat->data.stx2->n_units; idx++) {
                  switch (msg->resp_sysstat.sysstat->data.stx2->unit_stat[idx].unit.type) {
                    case ARCP_STX2_UNIT_EXT_COMBINER_SPLITTER:
                      len += (uint16)2;
                      len += msg->resp_sysstat.sysstat->data.stx2->unit_stat[idx].comb.n_temperatures;
                      len += (uint16)(4*msg->resp_sysstat.sysstat->data.stx2->unit_stat[idx].comb.n_outputs);
                      break;
                  }
                }
              }
              break;
            case ARCP_MODULE_BSM:
              len += (uint16)(11 + 2*msg->resp_sysstat.sysstat->data.bsm->n_fans);
              len += (uint16)(msg->resp_sysstat.sysstat->data.bsm->n_heatsink_temps);
              break;
          }
          break;
        default:
          /* All other solicited responses consist of only the header,
           * the response ID and information code.
           */
          len = ARCP_HEADER_SIZE + 4;
      }
      break;
    }
    default:
      len = 0;
  }

  msg->header.msg_length = len;
  return len;
}
/* ======================================================================== */

void arcp_msg_free(arcp_msg_t *msg) {
/*
 * Frees memory used by the given object.  For most message types this is
 * trivial, but some contain nested dynamic allocations which need to be
 * freed separately.
 */
  /* If the message is NULL there's nothing to do */
  if (msg == NULL)
    return;

  /* First deal with messages which could have allocated structures within */
  if (msg->header.msg_type == ARCP_MSG_COMMAND) {
    switch (msg->command.id) {
      case ARCP_CMD_SET_PULSE_SEQ:
        arcp_pulseseq_free(msg->cmd_set_pulse_seq.seq);
        break;
      case ARCP_CMD_SET_PULSE_PARAM:
        arcp_pulsecode_free(msg->cmd_set_pulse_param.pulse_param.code);
        break;
      case ARCP_CMD_SET_PHASE:
        free(msg->cmd_set_phase.phases);
        break;
    }
  } else
  if (msg->header.msg_type == ARCP_MSG_RESPONSE) {
    switch (msg->response.id) {
      case ARCP_RESP_SYSID:
        arcp_sysid_free(msg->resp_sysid.sysid);
        break;
      case ARCP_RESP_SYSSTAT:
        arcp_sysstat_free(msg->resp_sysstat.sysstat);
        break;
    }
  }
  free(msg);
}
/* ======================================================================== */

arcp_stream_t *arcp_stream_new(void) {
/*
 * Creates a new stream object used to store the bytes which need to be
 * sent on the wire.  The stream object is zeroed on creation.
 *
 * Notes:
 *  - arcp_stream_t::size is the total size allocated for the stream.
 */
  return calloc(1,sizeof(arcp_stream_t));
}
/* ======================================================================== */

signed int arcp_stream_error(arcp_stream_t *stream) {
/*
 * Return 1 if the stream has encountered an error condition, 0 if everything
 * is OK.
 */
  return stream->err != 0;
}
/* ======================================================================== */

signed int arcp_stream_get_int32(arcp_stream_t *stream, int32 *data) {
/*
 * Read a 32 bit integer from the head of the stream.  Returns 0 on success
 * or ARCP_ERROR_BADMSG if the end-of-stream prevents the operation from
 * completing.  The stream head is advanced past the 32 bits read before
 * returning.
 */
uint32 *sp = (uint32 *)(stream->head);
  if (stream->err || stream->head+4-1 > stream->end) {
    stream->err = 1;
    return ARCP_ERROR_BADMSG;
  }
  *data = htonl(*sp);
  stream->head += 4;
  return 0;
}
/* ======================================================================== */

signed int arcp_stream_get_int16(arcp_stream_t *stream, int16 *data) {
/*
 * Read a 16 bit integer from the head of the stream.  Returns 0 on success
 * or ARCP_ERROR_BADMSG if the end-of-stream prevents the operation from
 * completing.  The stream head is advanced past the 8 bits read before
 * returning.
 */
uint16 *sp = (uint16 *)(stream->head);
  if (stream->err || stream->head+2-1 > stream->end) {
    stream->err = 1;
    return ARCP_ERROR_BADMSG;
  }
  *data = htons(*sp);
  stream->head += 2;
  return 0;
}
/* ======================================================================== */

signed int arcp_stream_get_int8(arcp_stream_t *stream, int8 *data) {
/*
 * Read an 8 bit integer from the head of the stream.  Returns 0 on success
 * or ARCP_ERROR_BADMSG if the end-of-stream prevents the operation from
 * completing.  The stream head is advanced past the 8 bits read before
 * returning.
 */
  if (stream->err || stream->head+1-1 > stream->end) {
    stream->err = 1;
    return ARCP_ERROR_BADMSG;
  }
  *data = *(stream->head++);
  return 0;
}
/* ======================================================================== */

signed int arcp_stream_store_int32(arcp_stream_t *stream, int32 data) {
/*
 * Store a 32 bit integer into the stream at the head and move the head
 * to the next free location.  Returns 0 on success or ARCP_ERROR_BADMSG
 * if the end-of-stream is hit before the operation can complete.
 */
uint32 *dp = (uint32 *)(stream->head);
  if (stream->err || stream->head+4-1 > stream->end) {
    stream->err = 1;
    return ARCP_ERROR_BADMSG;
  }
  *dp = htonl(data);
  stream->head += 4;
  return 0;
}
/* ======================================================================== */

signed int arcp_stream_store_int16(arcp_stream_t *stream, int16 data) {
/*
 * Store a 16 bit integer into the stream at the head and move the head
 * if the end-of-stream is hit before the operation can complete.
 */
uint16 *dp = (uint16 *)(stream->head);
  if (stream->err || stream->head+2-1 > stream->end) {
    stream->err = 1;
    return ARCP_ERROR_BADMSG;
  }
  *dp = htons(data);
  stream->head += 2;
  return 0;
}
/* ======================================================================== */

signed int arcp_stream_store_int8(arcp_stream_t *stream, int8 data) {
/*
 * Store an 8 bit integer into the stream at the head and move the head
 * to the next free location.  Returns 0 on success or ARCP_ERROR_BADMSG
 * if the end-of-stream is hit before the operation can complete.
 */
  if (stream->err || stream->head+1-1 > stream->end) {
    stream->err = 1;
    return ARCP_ERROR_BADMSG;
  }
  *(stream->head++) = data;
  return 0;
}
/* ======================================================================== */
/* Create unsigned versions of the I/O functions to prevent warnings on
 * some particularly fussy windoze compilers.  Most compilers should be
 * quite happy with pointers to unsigned types being passed.
 */
#define arcp_stream_get_uint32(_s, _d) arcp_stream_get_int32(_s, (int32 *)(_d))
#define arcp_stream_get_uint16(_s, _d) arcp_stream_get_int16(_s, (int16 *)(_d))
#define arcp_stream_get_uint8(_s, _d) arcp_stream_get_int8(_s, (int8 *)(_d))

/* ======================================================================== */

signed int arcp_stream_get_float(arcp_stream_t *stream, float *data) {
/*
 * Read an IEEE 32-bit float from the head of the stream.  Returns 0 on
 * success or ARCP_ERROR_BADMSG if the end-of-stream prevents the operation from
 * completing.  The stream head is advanced past the 32 bits read before
 * returning.
 */
/* Use a union to avoid breaking strict aliasing rules */
union {
  uint32 i;
  float f;
} d;
signed int res = arcp_stream_get_uint32(stream, &d.i);

  if (res != 0)
    return res;

  *data = d.f;
  return 0;
}
/* ======================================================================== */

signed int arcp_stream_store_float(arcp_stream_t *stream, float data) {
/* 
 * Store a 32 bit IEEE float into the stream at the head and move the head
 * if the end-of-stream is hit before the operation can complete.
 */
/* Use a union to avoid strict aliasing problems */
union {
  uint32 i;
  float f;
} d;

  d.f = data;
  return arcp_stream_store_int32(stream, d.i);
}
/* ======================================================================== */

signed int arcp_stream_reset(arcp_stream_t *stream) {
/*
 * Resets the stream head to the start of the stream.  Returns 0 on success.
 * Currently this function does not fail.
 */
  stream->head = stream->data;
  stream->err = 0;
  return 0;
}
/* ======================================================================== */

signed int arcp_stream_setsize(arcp_stream_t *stream, uint16 newsize) {
/*
 * Sizes the given stream to accept up to newsize bytes.  Any existing
 * data in the stream is destroyed.  This function can be called with a
 * NULL stream pointer, in which case -1 will be returned to the caller.
 *
 * Returns 0 on success or an ARCP_ERROR_* if an error occurred.
 */
  /* Don't proceed if the given stream hasn't been allocated */
  if (stream == NULL)
    return ARCP_ERROR_LOCAL;

  /* If no resize is necessary simply return to the caller */
  if (newsize == stream->size)
    return 0;

  /* Deallocate any existing data the given stream might have */
  if (stream->data != NULL) {
    free(stream->data);
    stream->data = NULL;
  }

  /* Limit the streamsize to that specified in the protocol */
  if (newsize > ARCP_MSG_MAX_SIZE)
    return ARCP_ERROR_BADMSG;

  /* Allocate storage as requested and deal with any errors */
  if (newsize != 0) {
    stream->data = malloc(newsize);
  }
  if (newsize!=0 && stream->data==NULL) {
    stream->size = 0;
    return ARCP_ERROR_LOCAL;
  }
  
  stream->size = newsize;
  stream->head = stream->data;
  stream->end = stream->data + newsize-1;
  return 0;
}
/* ======================================================================== */

void arcp_stream_free(arcp_stream_t *stream) {
/*
 * Frees the given stream object.
 */
  if (stream == NULL)
    return;
  if (stream->data != NULL)
    free(stream->data);
  free(stream);
}
/* ======================================================================== */
/* Support functions for stream encoding functionality */

static signed int store_arcp_cmd(arcp_stream_t *stream, arcp_msg_t *msg) {
/*
 * Stores ARCP command message details into the given stream.  In the event
 * of stream overflow, the stream's error flag will be set on exit.  Return
 * values are 0 on success or an ARCP_ERROR_* code in the event of an error.
 */
int16 i;
signed int err = 0;
  arcp_stream_store_int16(stream, msg->command.id);
  switch (msg->command.id) {
    case ARCP_CMD_SET_MODULE_ENABLE:
      arcp_stream_store_int8(stream, msg->cmd_enable.enable);
      break;
    case ARCP_CMD_SET_PULSE_PARAM: {
      arcp_stream_store_int8(stream, msg->cmd_set_pulse_param.pulse_map_index);
      arcp_stream_store_int8(stream, msg->cmd_set_pulse_param.pulse_param.pulse_shape);
      arcp_stream_store_int16(stream, msg->cmd_set_pulse_param.pulse_param.pulse_ampl);
      arcp_stream_store_int16(stream, msg->cmd_set_pulse_param.pulse_param.pulse_options);
      arcp_stream_store_int32(stream, msg->cmd_set_pulse_param.pulse_param.pulse_width_ns);

      /* Write a pulse code if one is defined.  Otherwise just write a 
       * code length byte of 0 and move on.
       */
      if (msg->cmd_set_pulse_param.pulse_param.code!=NULL && 
          arcp_pulsecode_getlength(msg->cmd_set_pulse_param.pulse_param.code)!=0) {
        arcp_stream_store_int16(stream, 
          arcp_pulsecode_getlength(msg->cmd_set_pulse_param.pulse_param.code));
        /* Write the code bytes */
        i = 0;
        while (i<(arcp_pulsecode_getlength(msg->cmd_set_pulse_param.pulse_param.code)-1)/8 +1) {
          arcp_stream_store_int8(stream, msg->cmd_set_pulse_param.pulse_param.code->data[i]);
          i++;
        }
      } else {
        /* A NULL pulsecode is taken as a monopulse with a length of 0 */
        arcp_stream_store_int16(stream, 0);
      }
      break;
    }
    case ARCP_CMD_SET_PULSE_SEQ:
      if (msg->cmd_set_pulse_seq.seq->length > ARCP_MAX_PULSE_SEQ_LENGTH)
        err = ARCP_ERROR_BADMSG;
      else {
        arcp_stream_store_int16(stream, msg->cmd_set_pulse_seq.seq->length);
        for (i=0; i<msg->cmd_set_pulse_seq.seq->length; i++) {
          arcp_stream_store_int8(stream, msg->cmd_set_pulse_seq.seq->seq[i].slot);
          arcp_stream_store_int8(stream, msg->cmd_set_pulse_seq.seq->seq[i].flags);
        }
      }
      break;
    case ARCP_CMD_SET_PULSE_SEQ_IDX:
      arcp_stream_store_int16(stream, msg->cmd_set_pulse_seq_idx.seq_index);
      break;
    case ARCP_CMD_SET_TRIG_PARAM:
      arcp_stream_store_int8(stream, msg->cmd_set_trig_param.trig_param.trigger_source);
      arcp_stream_store_int8(stream, msg->cmd_set_trig_param.trig_param.ext_trigger_options);
      arcp_stream_store_int16(stream, msg->cmd_set_trig_param.trig_param.int_trigger_freq);
      arcp_stream_store_int16(stream, msg->cmd_set_trig_param.trig_param.pulse_predelay);
      break;
    case ARCP_CMD_SET_USRCTL_ENABLE:
      arcp_stream_store_int8(stream, msg->cmd_usrctl_enable.enable);
      break;

    case ARCP_CMD_SET_PHASE:
      if (msg->cmd_set_phase.n_phases > ARCP_BSM_MAX_N_PHASES)
        err = ARCP_ERROR_BADMSG;
      else {
        arcp_stream_store_int16(stream, msg->cmd_set_phase.phase_slot);
        arcp_stream_store_int16(stream, msg->cmd_set_phase.n_phases);
        for (i=0; i<msg->cmd_set_phase.n_phases; i++) {
          arcp_stream_store_int16(stream, msg->cmd_set_phase.phases[i].channel);
          arcp_stream_store_float(stream, msg->cmd_set_phase.phases[i].phase);
        }
      }
      break;

  }
  if (arcp_stream_error(stream))
    err = ARCP_ERROR_BADMSG;
  return err;
}

static signed int store_arcp_resp(arcp_stream_t *stream, arcp_msg_t *msg) {
/*
 * Stores ARCP response message details into the given stream.  If the stream
 * would have overflowed during this process the stream's error flag will
 * be set on return to the caller.  Return values are 0 on success or an
 * ARCP_ERROR_* code in the event of an error.
 */
int16 i,j;
signed int err = 0;
  arcp_stream_store_int16(stream, msg->response.id);
  arcp_stream_store_int16(stream, msg->response.info_code);
  switch (msg->response.id) {
    case ARCP_RESP_SYSID:
      arcp_stream_store_int8(stream, msg->resp_sysid.sysid->module_type);
      arcp_stream_store_int16(stream, msg->resp_sysid.sysid->module_version);
      arcp_stream_store_int16(stream, msg->resp_sysid.sysid->firmware_version);
      arcp_stream_store_int16(stream, msg->resp_sysid.sysid->ctrl_board_logic_version);
      switch (msg->resp_sysid.sysid->module_type) {
        case ARCP_MODULE_STX2:
          arcp_stream_store_int16(stream, msg->resp_sysid.sysid->data.stx2.card_map);
          arcp_stream_store_int32(stream, msg->resp_sysid.sysid->data.stx2.pulse_slot_length);
          break;
        case ARCP_MODULE_BSM:
          arcp_stream_store_int16(stream, msg->resp_sysid.sysid->data.bsm.channel_map);
          break;
      }
      break;
    case ARCP_RESP_SYSSTAT:
      arcp_stream_store_int8(stream, msg->resp_sysstat.sysstat->module_type);
      arcp_stream_store_int8(stream, msg->resp_sysstat.sysstat->module_status);
      switch (msg->resp_sysstat.sysstat->module_type) {
        case ARCP_MODULE_STX2:
          arcp_stream_store_int16(stream, msg->resp_sysstat.sysstat->data.stx2->status_code);
          arcp_stream_store_int8(stream, msg->resp_sysstat.sysstat->data.stx2->chassis_datasize);
          arcp_stream_store_int16(stream, msg->resp_sysstat.sysstat->data.stx2->rail_supply);
          arcp_stream_store_int16(stream, msg->resp_sysstat.sysstat->data.stx2->rail_aux);
          arcp_stream_store_int8(stream, msg->resp_sysstat.sysstat->data.stx2->ambient_temp);
          arcp_stream_store_int8(stream, msg->resp_sysstat.sysstat->data.stx2->n_chassis_fans);
          for (i=0; i<msg->resp_sysstat.sysstat->data.stx2->n_chassis_fans; i++) {
            arcp_stream_store_int16(stream, msg->resp_sysstat.sysstat->data.stx2->fan_speed[i]);
          }
          arcp_stream_store_int16(stream, msg->resp_sysstat.sysstat->data.stx2->card_map);
          arcp_stream_store_int8(stream, msg->resp_sysstat.sysstat->data.stx2->n_rf_cards);
          for (i=0; i<msg->resp_sysstat.sysstat->data.stx2->n_rf_cards; i++) {
            arcp_stream_store_int16(stream, msg->resp_sysstat.sysstat->data.stx2->rf_card_stat[i].rail_supply);
            arcp_stream_store_int16(stream, msg->resp_sysstat.sysstat->data.stx2->rf_card_stat[i].heatsink_temp);
            arcp_stream_store_int8(stream, msg->resp_sysstat.sysstat->data.stx2->rf_card_stat[i].n_rf_outputs);
            for (j=0; j<msg->resp_sysstat.sysstat->data.stx2->rf_card_stat[i].n_rf_outputs; j++) {
              arcp_stream_store_int16(stream, msg->resp_sysstat.sysstat->data.stx2->rf_card_stat[i].output_stat[j].forward_power);
              arcp_stream_store_int16(stream, msg->resp_sysstat.sysstat->data.stx2->rf_card_stat[i].output_stat[j].return_loss);
            }
          }
          /* Store information about additional STX2 units if present */
          arcp_stream_store_int8(stream, msg->resp_sysstat.sysstat->data.stx2->n_units);
          for (i=0; i<msg->resp_sysstat.sysstat->data.stx2->n_units; i++) {
            arcp_stream_store_int8(stream, msg->resp_sysstat.sysstat->data.stx2->unit_stat[i].unit.flags);
            arcp_stream_store_int8(stream, msg->resp_sysstat.sysstat->data.stx2->unit_stat[i].unit.type);
            switch (msg->resp_sysstat.sysstat->data.stx2->unit_stat[i].unit.type) {
              case ARCP_STX2_UNIT_EXT_COMBINER_SPLITTER:
                arcp_stream_store_int8(stream, msg->resp_sysstat.sysstat->data.stx2->unit_stat[i].comb.n_temperatures);
                for (j=0; j<msg->resp_sysstat.sysstat->data.stx2->unit_stat[i].comb.n_temperatures; j++) {
                  arcp_stream_store_int8(stream, msg->resp_sysstat.sysstat->data.stx2->unit_stat[i].comb.temperature[j]);
                }
                arcp_stream_store_int8(stream, msg->resp_sysstat.sysstat->data.stx2->unit_stat[i].comb.n_outputs);
                for (j=0; j<msg->resp_sysstat.sysstat->data.stx2->unit_stat[i].comb.n_outputs; j++) {
                  arcp_stream_store_int16(stream, msg->resp_sysstat.sysstat->data.stx2->unit_stat[i].comb.output[j].forward_power);
                  arcp_stream_store_int16(stream, msg->resp_sysstat.sysstat->data.stx2->unit_stat[i].comb.output[j].return_loss);
                }
                break;
            }
          }
          break;
       case ARCP_MODULE_BSM:
          arcp_stream_store_int16(stream, msg->resp_sysstat.sysstat->data.bsm->status_code);
          arcp_stream_store_int16(stream, msg->resp_sysstat.sysstat->data.bsm->rail_supply);
          arcp_stream_store_int16(stream, msg->resp_sysstat.sysstat->data.bsm->rail_aux);
          arcp_stream_store_int8(stream, msg->resp_sysstat.sysstat->data.bsm->ambient_temp);
          arcp_stream_store_int16(stream, msg->resp_sysstat.sysstat->data.bsm->channel_map);
          arcp_stream_store_int8(stream, msg->resp_sysstat.sysstat->data.bsm->n_fans);
          for (i=0; i<msg->resp_sysstat.sysstat->data.bsm->n_fans; i++) {
            arcp_stream_store_int16(stream, msg->resp_sysstat.sysstat->data.bsm->fan_speed[i]);
          }
          arcp_stream_store_int8(stream, msg->resp_sysstat.sysstat->data.bsm->n_heatsink_temps);
          for (i=0; i<msg->resp_sysstat.sysstat->data.bsm->n_heatsink_temps; i++) {
            arcp_stream_store_int8(stream, msg->resp_sysstat.sysstat->data.bsm->heatsink_temp[i]);
          }
          break;

      }
      break;
  }
  if (arcp_stream_error(stream))
    err = ARCP_ERROR_BADMSG;
  return err;
}
/* ======================================================================== */
/* Support functions for stream decoding functionality */

static signed int decode_arcp_cmd(arcp_stream_t *stream, arcp_msg_t *msg) {
/*
 * Decodes the command message details from the stream into the given message
 * structure.  Return values are 0 if successful, ARCP_ERROR_* codes on error.
 * If the stream underflowed during decoding the stream's error flag will
 * be set on exit.
 */
uint8 i;
signed int err = 0;
  arcp_stream_get_int16(stream, &msg->command.id);
  switch (msg->command.id) {
    case ARCP_CMD_SET_MODULE_ENABLE:
      arcp_stream_get_int8(stream, &msg->cmd_enable.enable);
      break;
    case ARCP_CMD_SET_PULSE_PARAM: {
      uint16 len = 0;
      arcp_stream_get_uint8(stream, &msg->cmd_set_pulse_param.pulse_map_index);
      arcp_stream_get_int8(stream, &msg->cmd_set_pulse_param.pulse_param.pulse_shape);
      arcp_stream_get_uint16(stream, &msg->cmd_set_pulse_param.pulse_param.pulse_ampl);
      arcp_stream_get_uint16(stream, &msg->cmd_set_pulse_param.pulse_param.pulse_options);
      arcp_stream_get_uint32(stream, &msg->cmd_set_pulse_param.pulse_param.pulse_width_ns);

      /* Extract the pulse code */
      arcp_stream_get_uint16(stream, &len);
      if (len > ARCP_MAX_PULSECODE_SIZE) 
        err = ARCP_ERROR_BADMSG;
      else
      if (len != 0) {
        msg->cmd_set_pulse_param.pulse_param.code = arcp_pulsecode_new(len);
        if (msg==NULL || arcp_pulsecode_setlength(msg->cmd_set_pulse_param.pulse_param.code, len) < 0)
          err = ARCP_ERROR_LOCAL;
        else {
          /* Load the bytes containing the pulse code */
          i = 0;
          while (i < 1+(len-1)/8) {
            arcp_stream_get_uint8(stream, &msg->cmd_set_pulse_param.pulse_param.code->data[i]);
            i++;
          }
        }
      } else
        msg->cmd_set_pulse_param.pulse_param.code = NULL;
      break;
    }
    case ARCP_CMD_SET_PULSE_SEQ: {
      uint16 len;
      arcp_stream_get_uint16(stream, &len);
      if (len > ARCP_MAX_PULSE_SEQ_LENGTH)
        err = ARCP_ERROR_BADMSG;
      else
      if ((msg->cmd_set_pulse_seq.seq = arcp_pulseseq_new(len)) == NULL)
        err = ARCP_ERROR_LOCAL;
      else {
        for (i=0; i<len; i++) {
          arcp_stream_get_uint8(stream, &msg->cmd_set_pulse_seq.seq->seq[i].slot);
          arcp_stream_get_uint8(stream, &msg->cmd_set_pulse_seq.seq->seq[i].flags);
        }
      }
      break;
    }
    case ARCP_CMD_SET_PULSE_SEQ_IDX:
      arcp_stream_get_uint16(stream, &msg->cmd_set_pulse_seq_idx.seq_index);
      break;
    case ARCP_CMD_SET_TRIG_PARAM:
      arcp_stream_get_uint8(stream, &msg->cmd_set_trig_param.trig_param.trigger_source);
      arcp_stream_get_uint8(stream, &msg->cmd_set_trig_param.trig_param.ext_trigger_options);
      arcp_stream_get_uint16(stream, &msg->cmd_set_trig_param.trig_param.int_trigger_freq);
      arcp_stream_get_uint16(stream, &msg->cmd_set_trig_param.trig_param.pulse_predelay);
      break;
    case ARCP_CMD_SET_USRCTL_ENABLE:
      arcp_stream_get_int8(stream, &msg->cmd_usrctl_enable.enable);
      break;

    case ARCP_CMD_SET_PHASE:
      arcp_stream_get_uint16(stream, &msg->cmd_set_phase.phase_slot);
      arcp_stream_get_uint16(stream, &msg->cmd_set_phase.n_phases);
      if (msg->cmd_set_phase.n_phases > ARCP_BSM_MAX_N_PHASES)
        return ARCP_ERROR_BADMSG;
      msg->cmd_set_phase.phases = malloc(sizeof(arcp_phase_entry_t)*msg->cmd_set_phase.n_phases);
      if (msg->cmd_set_phase.phases == NULL)
        return ARCP_ERROR_LOCAL;
      for (i=0; i<msg->cmd_set_phase.n_phases; i++) {
        arcp_stream_get_uint16(stream, &msg->cmd_set_phase.phases[i].channel);
        arcp_stream_get_float(stream, &msg->cmd_set_phase.phases[i].phase);
      }
      break;

  }
  if (arcp_stream_error(stream))
    err = ARCP_ERROR_BADMSG;
  return err;
}

static signed int decode_arcp_resp(arcp_stream_t *stream, arcp_msg_t *msg) {
/*
 * Decodes the response message details from the stream into the given message
 * structure.  Return values are 0 if successful, ARCP_ERROR_* codes on error.
 * If the stream underflowed during decoding the stream's error flag will
 * be set on exit.
 */
int16 i,j;
uint8 byte;
signed int err = 0;
  /* Read the response ID and set up required dynamic structures */
  arcp_stream_get_int16(stream, &i);
  if (arcp_msg_set_resp_id(msg, i) < 0) {
    return ARCP_ERROR_LOCAL;
  }
  arcp_stream_get_int16(stream, &msg->response.info_code);
  switch (msg->response.id) {
    case ARCP_RESP_SYSID:
      err = ((msg->resp_sysid.sysid = arcp_sysid_new()) == NULL);
      if (err) 
        break;
      arcp_stream_get_int8(stream, &msg->resp_sysid.sysid->module_type);
      arcp_stream_get_uint16(stream, &msg->resp_sysid.sysid->module_version);
      arcp_stream_get_uint16(stream, &msg->resp_sysid.sysid->firmware_version);
      arcp_stream_get_uint16(stream, &msg->resp_sysid.sysid->ctrl_board_logic_version);
      switch (msg->resp_sysid.sysid->module_type) {
        case ARCP_MODULE_STX2:
          arcp_stream_get_uint16(stream, &msg->resp_sysid.sysid->data.stx2.card_map);
          arcp_stream_get_uint32(stream, &msg->resp_sysid.sysid->data.stx2.pulse_slot_length);
          break;
        case ARCP_MODULE_BSM:
          arcp_stream_get_uint16(stream, &msg->resp_sysid.sysid->data.bsm.channel_map);
          break;
      }
      break;
    case ARCP_RESP_SYSSTAT:
      err = ((msg->resp_sysstat.sysstat = arcp_sysstat_new()) == NULL);
      if (err)
        break;
      arcp_stream_get_int8(stream, &msg->resp_sysstat.sysstat->module_type);
      arcp_stream_get_int8(stream, &msg->resp_sysstat.sysstat->module_status);
      switch (msg->resp_sysstat.sysstat->module_type) {
        case ARCP_MODULE_STX2: {
          arcp_stx2stat_t *stx2stat;
          byte = 0;
          if ((stx2stat = arcp_stx2stat_new()) == NULL) {
            err = ARCP_ERROR_LOCAL;
            break;
          }
          arcp_stream_get_uint16(stream, &stx2stat->status_code);
          arcp_stream_get_uint8(stream, &stx2stat->chassis_datasize);
          arcp_stream_get_uint16(stream, &stx2stat->rail_supply);
          arcp_stream_get_uint16(stream, &stx2stat->rail_aux);
          arcp_stream_get_int8(stream, &stx2stat->ambient_temp);
          arcp_stream_get_uint8(stream, &byte);
          /* Allow up to ARCP_MAX_N_CHASSIS_FANS chassis fans */
          if (byte > ARCP_MAX_N_CHASSIS_FANS)
            err = ARCP_ERROR_BADMSG;
          else
            err = arcp_stx2stat_set_n_chassis_fans(stx2stat, byte);
          if (err == 0) {
            for (i=0; i<stx2stat->n_chassis_fans; i++) {
              arcp_stream_get_uint16(stream, &stx2stat->fan_speed[i]);
            }
            arcp_stream_get_uint16(stream, &stx2stat->card_map);
            arcp_stream_get_uint8(stream, &byte);

            /* Allow up to ARCP_MAX_N_RF_CARDS RF cards */
            if (byte > ARCP_MAX_N_RF_CARDS)
              err = ARCP_ERROR_BADMSG;
            else
              err = arcp_stx2stat_set_n_rf_cards(stx2stat, byte);
          }
          if (err == 0) {
            for (i=0; i<stx2stat->n_rf_cards; i++) {
              arcp_stream_get_uint16(stream, &stx2stat->rf_card_stat[i].rail_supply);
              arcp_stream_get_int16(stream, &stx2stat->rf_card_stat[i].heatsink_temp);
              arcp_stream_get_uint8(stream, &byte);
              /* Allow up to ARCP_MAX_N_RF_CARD_OUTPUT outputs per RF card */
              if (byte > ARCP_MAX_N_RF_CARD_OUTPUT)
                err = ARCP_ERROR_BADMSG;
              else
                err = arcp_stx2stat_set_n_rf_outputs(stx2stat,i,byte);
              if (err == 0) {
                for (j=0; j<stx2stat->rf_card_stat[i].n_rf_outputs; j++) {
                  arcp_stream_get_uint16(stream, &stx2stat->rf_card_stat[i].output_stat[j].forward_power);
                  arcp_stream_get_int16(stream, &stx2stat->rf_card_stat[i].output_stat[j].return_loss);
                }
              }
            }
          }
          byte = 0;
          /* Read the flag byte indicating the presence of an external
           * unit record.
           */
          if (err == 0)
            arcp_stream_get_uint8(stream, &byte);
          /* Read information about the external units if present */
          if (byte > ARCP_STX2_MAX_N_STX2_UNITS)
            err = ARCP_ERROR_BADMSG;
          else
          if (arcp_stx2stat_set_n_units(stx2stat, byte) != 0) {
            err = ARCP_ERROR_LOCAL;
          }
          for (i=0; i<stx2stat->n_units && err==0; i++) {
            arcp_stream_get_uint8(stream, &stx2stat->unit_stat[i].unit.flags);
            arcp_stream_get_uint8(stream, &stx2stat->unit_stat[i].unit.type);
            switch (stx2stat->unit_stat[i].unit.type) {
              case ARCP_STX2_UNIT_EXT_COMBINER_SPLITTER:
                arcp_stream_get_uint8(stream, &byte);
                if (byte > ARCP_STX2_EXTCOMB_MAX_N_TEMPERATURES)
                  err = ARCP_ERROR_BADMSG;
                else {
                  stx2stat->unit_stat[i].comb.n_temperatures = byte;
                  for (j=0; j<stx2stat->unit_stat[i].comb.n_temperatures; j++) {
                    arcp_stream_get_int8(stream, &stx2stat->unit_stat[i].comb.temperature[j]);
                  }
                }
                if (err == 0)
                  arcp_stream_get_uint8(stream, &byte);
                if (err==0 && byte>ARCP_STX2_EXTCOMB_MAX_N_OUTPUTS)
                  err = ARCP_ERROR_BADMSG;
                if (err == 0) {
                  stx2stat->unit_stat[i].comb.n_outputs = byte;
                  for (j=0; j<stx2stat->unit_stat[i].comb.n_outputs; j++) {
                    arcp_stream_get_uint16(stream, &stx2stat->unit_stat[i].comb.output[j].forward_power);
                    arcp_stream_get_int16(stream, &stx2stat->unit_stat[i].comb.output[j].return_loss);
                  }
                }
                break;
            }
          }

          if (err == 0)
            msg->resp_sysstat.sysstat->data.stx2 = stx2stat;
          else
            arcp_stx2stat_free(stx2stat);
          break;
        }

        case ARCP_MODULE_BSM: {
          arcp_bsmstat_t *bsmstat;
          byte = 0;
          if ((bsmstat = arcp_bsmstat_new()) == NULL) {
            err = ARCP_ERROR_LOCAL;
            break;
          }
          arcp_stream_get_uint16(stream, &bsmstat->status_code);
          arcp_stream_get_uint16(stream, &bsmstat->rail_supply);
          arcp_stream_get_uint16(stream, &bsmstat->rail_aux);
          arcp_stream_get_int8(stream, &bsmstat->ambient_temp);
          arcp_stream_get_uint16(stream, &bsmstat->channel_map);

          arcp_stream_get_uint8(stream, &byte);
          if (byte > ARCP_MAX_N_CHASSIS_FANS)
            err = ARCP_ERROR_BADMSG;
          else
            err = arcp_bsmstat_set_n_fans(bsmstat, byte);
          if (err == 0) {
            for (i=0; i<bsmstat->n_fans; i++) {
              arcp_stream_get_uint16(stream, &bsmstat->fan_speed[i]);
            }
            arcp_stream_get_uint8(stream, &byte);
            /* Allow up to ARCP_BSM_MAX_N_TEMPERATURES temperatures */
            if (byte > ARCP_BSM_MAX_N_TEMPERATURES) {
              err = ARCP_ERROR_BADMSG;
            } else
              bsmstat->n_heatsink_temps = byte;
            for (i=0; i<bsmstat->n_heatsink_temps && err==0; i++) {
              arcp_stream_get_int8(stream, &bsmstat->heatsink_temp[i]);
            }
          }
          if (err == 0)
            msg->resp_sysstat.sysstat->data.bsm = bsmstat;
          else
            arcp_bsmstat_free(bsmstat);
          break;
        }

    }
  }
  if (arcp_stream_error(stream))
    err = ARCP_ERROR_BADMSG;
  return err;
}
/* ======================================================================== */

signed int arcp_stream_decode(arcp_stream_t *stream, arcp_msg_t **dec_msg) {
/*
 * Decodes the given ARCP stream into a new arcp_msg_t structure.  Returns 0
 * on success or an ARCP_ERROR_* on failure.  In event of an error, *dec_msg
 * will be NULL on return.
 */
arcp_msgtype_t msg_type;
uint32 magic_num;
uint16 msg_length, xchg_id;
arcp_msg_t *msg;
signed int res = 0;

  /* If the caller hasn't provided storage for the return value, abort */
  if (dec_msg == NULL)
    return ARCP_ERROR_INTERNAL;

  /* Set the return value to a reasonable default */
  *dec_msg = NULL;

  /* Every ARCP message has an ARCP header and therefore must be at least 
   * 11 bytes long.  Furthermore, the current version of the ARCP limits
   * messages to a maximum size of ARCP_MSG_MAX_SIZE.
   */
  if (stream->size<11 || stream->size>ARCP_MSG_MAX_SIZE) {
    return ARCP_ERROR_BADMSG;
  }

  /* Read in most of the header information */
  arcp_stream_get_uint32(stream, &magic_num);
  arcp_stream_get_uint16(stream, &msg_length);
  arcp_stream_get_uint16(stream, &xchg_id);
  arcp_stream_get_uint8(stream, &msg_type);

  /* Use information from the header to construct an arcp_msg_t object */
  msg = arcp_msg_new(msg_type);

  if (msg == NULL) {
    return ARCP_ERROR_LOCAL;
  }
  msg->header.magic_num = magic_num;
  msg->header.msg_length = msg_length;
  msg->header.exchange_id = xchg_id;

  /* Complete the reading of the header data */
  arcp_stream_get_uint16(stream, &msg->header.protocol_version);

  /* Now get the message-specific details */
  switch (msg->header.msg_type) {
    case ARCP_MSG_COMMAND:
      res = decode_arcp_cmd(stream, msg);
      break;
    case ARCP_MSG_RESPONSE:
      res = decode_arcp_resp(stream, msg);
      break;
  }

  /* Deal with any errors encountered during the decoding of the stream */
  if (res < 0) {
    arcp_msg_free(msg);
  } else
    *dec_msg = msg;

  return res;
}
/* ======================================================================== */

signed int arcp_msg_encode(arcp_msg_t *msg, arcp_stream_t **enc_stream) {
/*
 * Encodes the given ARCP message into a new ARCP stream object.  Returns 0
 * on success or an ARCP_ERROR_* code on failure.  If successful,
 * *stream_final will be used to return the encoded stream; otherwise it
 * will be set to NULL.  The primary potential cause of an error is a
 * failure to allocate space for the stream object.
 */
arcp_stream_t *stream;
uint16 msg_size = arcp_msg_set_stream_size(msg);
signed int res = 0;

  /* Check for programming errors and invalid message requests */
  if (enc_stream == NULL)
    return ARCP_ERROR_INTERNAL;
  if (msg_size==0 || msg_size>ARCP_MSG_MAX_SIZE)
    return ARCP_ERROR_BADMSG;

  /* Set the output parameter to something sensible */
  *enc_stream = NULL;

  /* Allocate space for the new stream */
  stream = arcp_stream_new();
  if (stream == NULL)
    return ARCP_ERROR_LOCAL;

  /* Allocate storage for the new message stream */
  if (arcp_stream_setsize(stream, msg_size) < 0) {
    arcp_stream_free(stream);
    return ARCP_ERROR_LOCAL;
  }

  /* Construct the ARCP header - common to all messages */
  arcp_stream_store_int32(stream, msg->header.magic_num);
  arcp_stream_store_int16(stream, msg->header.msg_length);
  arcp_stream_store_int16(stream, msg->header.exchange_id);
  arcp_stream_store_int8(stream, msg->header.msg_type);
  arcp_stream_store_int16(stream, msg->header.protocol_version);

  /* Now put the message-specific details in */
  switch (msg->header.msg_type) {
    case ARCP_MSG_COMMAND:
      res = store_arcp_cmd(stream, msg);
      break;
    case ARCP_MSG_RESPONSE:
      res = store_arcp_resp(stream, msg);
      break;
  }

  if (res < 0) {
    arcp_stream_free(stream);
  } else
    *enc_stream = stream;

  return res;
}
/* ======================================================================== */

static signed int read_from_socket(arcp_socket_t fd, void *buf, size_t len, 
  int flags) {
/*
 * This function acts as a wrapper around arcp_socket_read(); it avoids the
 * need for replicating the boilerplate error handing code in multiple
 * places.
 *
 * Return value is the number of bytes read on success or an
 * ARCP_ERROR_CONN_* code in the case of an error.
 */
signed int i;
size_t count=0;

  while (count < len) {
    /* Typecast to (char *) is required by some windoze compilers which
     * refuse to assume sizeof(void) == 1.
     */
    i = arcp_socket_read(fd, ((char *)buf)+count, len-count, flags);
    /* This series of conditionals allow for resumption of interrupted
     * system calls and a subtle difference between the return values from
     * NutOS' NutTcpReceive() and BSD recv() functions in the case of a
     * socket timeout.
     */
#ifdef ARCP_NUTOS
    if (i==0)
      return ARCP_ERROR_CONN_TIMEOUT;
    if (i<0 && SOCKET_ERRNO(fd)!=EINTR)
      return ARCP_ERROR_CONN_DROPPED;
#else
    if (i<0 && SOCKET_ERRNO(fd)==EWOULDBLOCK)
      return ARCP_ERROR_CONN_TIMEOUT;
    if (i==0 || (i<0 && SOCKET_ERRNO(fd)!=EINTR))
      return ARCP_ERROR_CONN_DROPPED;
#endif
    if (i > 0)
      count += i;
  }

  return count;
}
/* ======================================================================== */

static signed int arcp_socket_process(arcp_socket_t fd, arcp_stream_t **stream,
  unsigned char **ascii_msg) {
/*
 * Internal function: reads an ARCP stream or ASCII message from the given
 * socket.  This function attempts to be reasonably intelligent in that it
 * will ensure that the returned stream is valid and complete.  This
 * insulates any users of this function from the effect of transport delays
 * from TCP/IP, and allows error recovery in the event that the reader gets
 * out of sync with the incoming byte stream.
 *
 * The mode used depends on which storage locations have been provided by
 * the caller.  If stream is given, ARCP streams will be read if present,
 * and if ascii_msg is given, ASCII messages will be allowed.  Both
 * stream and ascii_msg can be given, in which case the function will
 * search for both, returning the first one identified.
 *
 * Returns 0 on success (with *stream pointing to a newly created stream
 * object OR *ascii_msg pointing to a newly created ascii message).  In
 * event of error an ARCP_ERROR_* code will be returned.
 *
 * TODO: implement timeouts, possibly using select().
 */
unsigned char *c;
arcp_stream_t *local_stream;
signed int i;
uint32 magic_num = 0;
uint16 msg_size = 0;
uint8 byte = 0;
uint8 flags = 0;
uint16 len;

  /* If the caller didn't supply a pointer variable in which to reference
   * the resultant stream, there's no point in continuing.
   */
  if (stream==NULL && ascii_msg==NULL)
    return ARCP_ERROR_INTERNAL;

  /* Set the return pointers to sensible defaults and set the operation
   * mode.
   */
  if (stream != NULL) {
    *stream = NULL;
    flags |= MSG_ARCP;
  }
  if (ascii_msg != NULL) {
    *ascii_msg = NULL;
    flags |= MSG_ASCII;
  }

  /* Locate the start of a message.  There are two modes which can be run.
   * MSG_ARCP searches for the start of a message based on the ARCP magic
   * number, while MSG_ASCII searches for an ASCII message which is assumed
   * to be no longer than 4 characters long including the newline character.
   * Both modes can be active at once, but normally such an activation would
   * only be requested for the first call to this function for a given 
   * connection.  Thereafter more reliable operation will result from
   * calls where flags is set to the specific mode detected in the first
   * call.
   *
   * In ARCP mode, the search for the magic number in the incoming stream
   * effectively acts as a synchroniser.  TCP's guaranteed delivery should
   * make this unnecessary, but having this means that the protocol can
   * recover if there has been a parsing error earlier.  Note that the
   * incoming bytes are in network byte order - that is, big-endian.
   */
  msg_size = 0;
  while ( (!(flags & MSG_ARCP) || magic_num!=ARCP_MAGIC_NUMBER) && msg_size<4 &&
          (!(flags & MSG_ASCII) || byte!='\n') ) {
    i = read_from_socket(fd, &byte, 1, 0);
    if (i < 0)
      return i;

    magic_num = (magic_num << 8) | byte;
    if (msg_size < 4)
      msg_size++;
  }
  if ( (!(flags & MSG_ARCP) || magic_num!=ARCP_MAGIC_NUMBER) &&
       (!(flags & MSG_ASCII) || byte!='\n') ) {
    return ARCP_ERROR_BADMSG;
  }

  /* If an ASCII message has been detected, return it straight away since
   * there's nothing else to do with it.
   */
  if ((flags & MSG_ASCII) && byte=='\n') {
    /* Work out the length of the ASCII message including a NULL terminator
     * but excluding any CR/LF characters which might have been sent.
     * The last character read must be a LF by definition.  The one before
     * it may or may not be a CR depending on the program used to connect.
     */
    msg_size--;
    magic_num >>= 8;
    if ((magic_num & 0xff) == '\r') {
      msg_size--;
      magic_num >>= 8;
    }
    *ascii_msg = malloc(msg_size+1);
    if (*ascii_msg == NULL)
      return ARCP_ERROR_LOCAL;
    c = *ascii_msg;
    i = 0;
    while (i<msg_size) {
     /* Typecast required to counter a warning from some silly windoze
      * compilers which promote the type of the expression for some odd
      * reason.
      */
      *c = (unsigned char)((magic_num >> ((msg_size-1-i)*8)) & 0xff);
      i++;
      c++;
    }
    /* Store terminating NULL character */
    *c = 0;
    return 0;
  }

  /* At this point a valid magic number has been read, so the next two bytes 
   * should indicate the total message size in bytes.
   */
  i = read_from_socket(fd, &msg_size, 2, 0);
  if (i < 0)
    return i;

  /* msg_size will be in network byte order as it comes off the wire */
  msg_size = htons(msg_size);

  /* Sanity-check the message size.  By definition it must be greater than
   * or equal to 11.  We also place an upper bound of ARCP_MSG_MAX_SIZE on
   * the size.
   */
  if (msg_size<=11 || msg_size>ARCP_MSG_MAX_SIZE) {
    return ARCP_ERROR_BADMSG;
  }

  /* Everything has checked out, so prepare to read the rest of the message
   * from the stream.
   */
  local_stream = arcp_stream_new();
  if (local_stream == NULL)
    return ARCP_ERROR_LOCAL;
  if (arcp_stream_setsize(local_stream,msg_size) < 0) {
    arcp_stream_free(local_stream);
    return ARCP_ERROR_LOCAL;
  }
  /* Put the two data fields already read into the new message stream 
   * object.
   */
  arcp_stream_store_int32(local_stream, magic_num);
  arcp_stream_store_int16(local_stream, msg_size);

  /* The number of bytes left to read is <msg_size>-4-2; 4 for the magic
   * number and 2 for the message size.  Read these bytes straight into
   * the stream object.  Allow for partial transfers.
   */
  len = 6;
  i = read_from_socket(fd, local_stream->data+len, msg_size-len, 0);
  if (i<0 || len+i!=msg_size) {
    arcp_stream_free(local_stream);
    return ARCP_ERROR_BADMSG;
  }
  local_stream->head += i;

  /* Reset the stream pointer so it's ready to be read */
  arcp_stream_reset(local_stream);

  *stream = local_stream;
  return 0;
}
/* ======================================================================== */

arcp_handle_t *arcp_handle_new(arcp_socket_t fd) {
/*
 * Creates a new ARCP handle and associates it with the given socket.
 * Returns a new handle on success, or NULL if an error occurred.
 */
arcp_handle_t *h = calloc(1, sizeof(arcp_handle_t));

  if (h == NULL)
    return NULL;
  h->fd = fd;

  /* Assume the current protocol version is appropriate */
  h->connection_arcp_version = ARCP_VERSION_WORD(ARCP_VERSION_MAJOR, ARCP_VERSION_MINOR);
  return h;
}
/* ======================================================================== */

arcp_socket_t arcp_handle_get_socket(arcp_handle_t *handle) {
/*
 * Returns the socket identifier associated with the given handle.  On
 * error, (arcp_socket_t)0 is returned.
 */
  if (handle != NULL) {
    return handle->fd;
  }
  return (arcp_socket_t)0;
}
/* ======================================================================== */

uint16 arcp_handle_get_connection_arcp_version(arcp_handle_t *handle) {
/*
 * Returns the connection version of the ARCP connection associated with
 * the given handle.  On error, 0 is returned.
 */
  if (handle != NULL) {
    return handle->connection_arcp_version;
  }
  return 0;
}
/* ======================================================================== */

void arcp_handle_free(arcp_handle_t *handle) {
/*
 * Closes down an ARCP handle and frees memory used by it.  Note that the
 * associated socket is *not* closed down; ideally this should be done by
 * the caller prior to calling this function.
 */
  if (handle!=NULL) {
    free(handle);
  }
}
/* ======================================================================== */

signed int arcp_stream_read(arcp_handle_t *handle, arcp_stream_t **stream) {
/*
 * Reads an ARCP stream message from the given ARCP connection.  This
 * function attempts to be reasonably intelligent in that it will ensure
 * that the returned stream is valid and complete.  This insulates any users
 * of this function from the effect of transport delays from TCP/IP, and
 * allows error recovery in the event that the reader gets out of sync with
 * the incoming byte stream.
 *
 * Returns 0 on success (with *stream pointing to a newly created stream
 * object).  In event of error an ARCP_ERROR_* code will be returned.
 */
  return arcp_socket_process(handle->fd, stream, NULL);
}
/* ======================================================================== */

signed int arcp_stream_write(arcp_handle_t *handle, arcp_stream_t *stream) {
/*
 * Sends the given ARCP stream contents to the given ARCP connection. 
 * Returns 0 on success or ARCP_ERROR_CONN_DROPPED if an error occurs (which
 * is assumed to be caused by a connection dropout).
 */

size_t send_cx, len;
int n_sent;

  /* send() (or NutTcpSend() under NutOS) doesn't guarantee to send all
   * the bytes in the one call.  Therefore loop until all bytes are sent
   * or an error occurs.
   */
  len = stream->size;
  send_cx = 0;
  n_sent = 0;

  while (send_cx<len && n_sent>=0) {
    n_sent = arcp_socket_write(handle->fd, stream->data+send_cx, len-send_cx, MSG_NOSIGNAL);
    if (n_sent > 0)
      send_cx += n_sent;

    /* Deal with timeouts if the socket has been configured with a recv
     * timeout using setsockopts(..., SOL_SOCKET, SO_RCVTIMEO, ...). 
     * Unfortunately there is some platform-specific variation here.
     * NutOS recv-like function returns 0 to indicate timeout; other
     * platforms return -1 with errno set to EWOULDBLOCK while a return value
     * of 0 indicates a cleanly closed connection.  Also don't forget the 
     * possibility of interrupted system calls.
     */
#ifdef ARCP_NUTOS
    if (n_sent == 0)
      return ARCP_ERROR_CONN_TIMEOUT;
#else
    if (n_sent<0 && SOCKET_ERRNO(handle->fd)==EWOULDBLOCK)
      return ARCP_ERROR_CONN_TIMEOUT;
    if (n_sent == 0)
      return ARCP_ERROR_CONN_DROPPED;
#endif
    if (n_sent<0 && SOCKET_ERRNO(handle->fd)==EINTR)
      n_sent = 0;
  }
  if (send_cx < len)
    return ARCP_ERROR_CONN_DROPPED;

  return 0;
}
/* ======================================================================== */

signed int arcp_ascii_or_arcp_read(arcp_handle_t *handle, arcp_msg_t **msg_read,
  unsigned char **ascii_read) {
/*
 * Attempts to read either an ARCP message or an ASCII message from the
 * given ARCP connection handle.  Both msg_read and ascii_read must be
 * supplied by the caller for full functionality; otherwise the call becomes
 * equivalent to arcp_ascii_read() or arcp_msg_read().  The type of message
 * discovered will be detectable in the caller by testing which output
 * parameter (msg_read or ascii_read) is not NULL.
 *
 * Return value is 0 on success or an ARCP_ERROR_* code otherwise.
 */

arcp_stream_t *stream;
signed int res;
arcp_msg_t *msg;

  /* Default the return messages to something sensible */
  if (msg_read != NULL)
    *msg_read = NULL;
  if (ascii_read != NULL)
    *ascii_read = NULL;

  /* Call arcp_socket_process() to do the reading from the socket.  Only
   * pass a pointer for an ARCP stream if the caller has provided somewhere
   * to store the resulting message.
   */
  res = arcp_socket_process(handle->fd, msg_read==NULL?NULL:&stream, ascii_read);

  /* If there was an error while reading from the socket, return immediately.
   * Neither *stream nor *ascii will be allocated in this case. 
   */
  if (res != 0) {
    return res;
  }

  /* If an ASCII message was read, return it straight away */
  if (ascii_read!=NULL && *ascii_read != NULL) {
    return 0;
  }

  /* An ARCP stream was read, so attempt to decode it.  Since stream was
   * only passed to arcp_socket_process() if msg_read has been supplied 
   * by the caller we don't have to check the validity of msg_read any
   * more. */
  res = arcp_stream_decode(stream, &msg);
  arcp_stream_free(stream);

  if (res == 0)
    *msg_read = msg;

  return res;
}
/* ======================================================================== */

signed int arcp_ascii_read(arcp_handle_t *handle, unsigned char **ascii) {
/*
 * Reads an ASCII message from the given ARCP handle.  On return, *ascii
 * contains the ASCII message or is NULL if any error occurred.  Return
 * value is 0 on success or an ARCP_ERROR_* code otherwise.
 */
signed int res = arcp_socket_process(handle->fd, NULL, ascii);
  return res;
}    
/* ======================================================================== */

signed int arcp_msg_read(arcp_handle_t *handle, arcp_msg_t **msg_read) {
/*
 * Reads a decoded ARCP message from the given ARCP handle.  On return,
 * *msg_read contains the ARCP message or is NULL if any error occurred. 
 * Return value is 0 on success or an ARCP_ERROR_* code otherwise.
 */
signed int result;

  /* If the caller hasn't provided somewhere to store the result there's 
   * no point in continuing.
   */
  if (msg_read == NULL)
    return ARCP_ERROR_INTERNAL;

  result = arcp_ascii_or_arcp_read(handle, msg_read, NULL);
  if (result == 0) {
    /* Override the connection's ARCP version if the incoming response is
     * from an earlier version of the protocol.
     */
    if ((*msg_read)->header.protocol_version < handle->connection_arcp_version)
      handle->connection_arcp_version = (*msg_read)->header.protocol_version;
  }

  return result;
}    
/* ======================================================================== */

signed int arcp_msg_write(arcp_handle_t *handle, arcp_msg_t *msg) {
/*
 * Sends the given ARCP message to the supplied ARCP handle.  Returns 0
 * on success or an ARCP_ERROR_* code on error.
 */
arcp_stream_t *stream;
signed int i;

  /* Set the message's protocol version to that of the connection in use */
  msg->header.protocol_version = handle->connection_arcp_version;

  i = arcp_msg_encode(msg, &stream);
  if (i != 0)
    return i;
  i = arcp_stream_write(handle, stream);
  arcp_stream_free(stream);
  return i;
}
/* ======================================================================== */

signed int arcp_check_resp_msg(arcp_msg_t *cmd, arcp_msg_t *resp) {
/*
 * Checks the "resp" message against the "cmd" message for consistency.
 * "resp" is assumed to be a message received from a slave in response to
 * the "cmd" message.
 *
 * It is assumed that "cmd" was internally generated and that no consistency
 * checking is required of it.
 *
 * Return value is 0 if everything checks out or an ARCP_ERROR_* code if
 * a problem was detected.
 */

  /* Cover for the case where a local error (most likely memory allocation
   * failure) means that no message has been stored in response.
   */
  if (resp == NULL)
    return ARCP_ERROR_LOCAL;

  /* Perform a general consistency check in the header */
  if (resp->header.magic_num!=ARCP_MAGIC_NUMBER) {
    return ARCP_ERROR_BADMSG;
  }

  if (resp->header.msg_type!=ARCP_MSG_RESPONSE) {
    return ARCP_ERROR_NOT_RESP;
  }

  /* Make sure the response really is an answer to the given command */
  if (resp->header.exchange_id != cmd->header.exchange_id) {
    return ARCP_ERROR_SEQUENCE;
  }

  /* It's possible to be backwards compatible to earlier protocol versions,
   * but compatibility with future versions is almost impossible to
   * achieve.
   */
  if (cmd->header.protocol_version < resp->header.protocol_version)
    return ARCP_ERROR_BAD_PROTO_VER;

  return 0;
}
/* ======================================================================== */

static signed int arcp_exec_cmd(arcp_handle_t *handle, arcp_cmd_id_t cmd_id,
  arcp_msg_t *cmd_msg, arcp_msg_t **resp_ret) {
/*
 * Internal function.  Prepares and sends a command to the slave attached to
 * the given ARCP handle (handle).  Two variations are supported:
 *   - a simple command comprising just of the supplied cmd_id.  Currently
 *     the following commands are regarded as simple:
 *       ARCP_CMD_RESET, ARCP_CMD_PING, ARCP_CMD_GET_SYSID, 
 *       ARCP_CMD_GET_SYSSTAT
 *   - a complex command, utilising a pre-prepared ARCP message.  In this case
 *     cmd_msg should be set to the command ID which is expected in the
 *     supplied message to allow it to be checked.
 *
 * A simple command is assumed if cmd_msg is NULL.  Otherwise the command
 * message sent will be that provided by the caller in cmd_msg.
 *
 * If an inappropriate command ID has been supplied for use in as a simple
 * command, ARCP_ERROR_INTERNAL will be returned.  ARCP_ERROR_LOCAL is
 * returned if there was an error creating the simple command.
 *
 * The return value is normally the response ID received from the targetted
 * slave.  Errors are flagged by the return of an ARCP_ERROR_* code.
 *
 * If not NULL, the response packet received is returned via *resp_ret.
 * Otherwise it is destroyed before returning.
 *
 * If supplied, cmd_msg is NOT deallocated by this function.
 */
arcp_msg_t *cmd_to_send;
arcp_msg_t *resp;
signed err;
int8 resp_id = 0;

  resp = NULL;

  /* Prepare a simple command if a pre-prepared message hasn't been supplied */
  if (cmd_msg == NULL) {
    /* Abort if an inappropriate command ID has been requested as a simple
     * command.
     */
    if (cmd_id!=ARCP_CMD_RESET && cmd_id!=ARCP_CMD_PING &&
        cmd_id!=ARCP_CMD_GET_SYSID && cmd_id!=ARCP_CMD_GET_SYSSTAT) {
      return ARCP_ERROR_INTERNAL;
    }

    /* Prepare an ARCP message to send */
    cmd_to_send = arcp_msg_new(ARCP_MSG_COMMAND);
    if (cmd_to_send == NULL)
      return ARCP_ERROR_LOCAL;
    cmd_to_send->command.id = cmd_id;
  } else {
    /* If the supplied message is not a command, abort with
     * ARCP_ERROR_INTERNAL.
     */
    if (cmd_msg->header.msg_type!=ARCP_MSG_COMMAND ||
        cmd_msg->command.id != cmd_id) {
      return ARCP_ERROR_INTERNAL;
    }

    cmd_to_send = cmd_msg;
  }
  /* Assign an appropriate exchange ID */
  cmd_to_send->header.exchange_id = exchange_id++;

  /* Send the message */
  err = arcp_msg_write(handle,cmd_to_send);

  /* If no errors, attempt to read a response message and check it */
  if (err == 0) {
    err = arcp_msg_read(handle, &resp);
  }
  if (err == 0) {
    err = arcp_check_resp_msg(cmd_to_send,resp);
  }

  /* If everything checks out, get the response ID and free the messages
   * in preparation for a return.
   */
  if (err == 0)
    resp_id = resp->response.id;

  /* Free the command message if it was not supplied in pre-prepared form */
  if (cmd_msg == NULL)
    arcp_msg_free(cmd_to_send);

  /* If the caller has requested the response message be returned, do so.
   * Otherwise just free it.
   */
  if (resp_ret != NULL)
    *resp_ret = resp;
  else 
  if (resp != NULL)
    arcp_msg_free(resp);
  
  return (err!=0)?err:resp_id;
}
/* ======================================================================== */

static signed int arcp_exec_resp(arcp_handle_t *handle, arcp_msg_t *orig_cmd, 
  arcp_resp_id_t resp_id, arcp_msg_t *resp_msg, int16 info_code) {
/*
 * Internal function.  Prepares and sends a response to the ARCP node
 * attached to the given ARCP handle (handle) in response to the supplied
 * command message.  Two variations are supported:
 *   - a simple response comprising just of the supplied resp_id.  Currently
 *     the following responses are regarded as simple:
 *       ARCP_RESP_UNK, ARCP_RESP_NAK, ARCP_RESP_ACK
 *   - a complex response, utilising a pre-prepared ARCP message.  In this
 *     case resp_id should be set to the ID expected to allow the type of
 *     the supplied response to be checked.
 *
 * info_code contains additional information passed in the response packet's
 * info_code field; it is used only if resp_msg is not supplied.
 *
 * A simple response is assumed if resp_msg is NULL.  Otherwise the response
 * message sent will be that provided by the caller in resp_msg.
 *
 * If an inappropriate response ID has been supplied for use in as a simple
 * response, ARCP_ERROR_INTERNAL will be returned.  The same error is returned
 * if there was an error creating the simple response.
 *
 * The return value is 0 if no errors occured during the send or an
 * ARCP_ERROR_* code if errors were encountered.
 *
 * orig_cmd is NOT deallocated by this function.
 */
arcp_msg_t *resp_to_send;
signed err;

  /* If no command message has been supplied to respond to, flag an
   * internal error.
   */
  if (orig_cmd==NULL || orig_cmd->header.msg_type!=ARCP_MSG_COMMAND)
    return ARCP_ERROR_INTERNAL;

  /* Prepare a simple response if a pre-prepared message hasn't been 
   * supplied. */
  if (resp_msg == NULL) {
    /* Abort if an inappropriate response ID has been requested as a simple
     * response.
     */
    if (resp_id!=ARCP_RESP_UNK && resp_id!=ARCP_RESP_NAK && 
        resp_id!=ARCP_RESP_ACK) {
      return ARCP_ERROR_INTERNAL;
    }

    /* Prepare an ARCP message to send */
    resp_to_send = arcp_msg_new(ARCP_MSG_RESPONSE);
    if (resp_to_send == NULL)
      return ARCP_ERROR_INTERNAL;

    resp_to_send->response.id = resp_id;
    resp_to_send->response.info_code = info_code;
  } else {
    /* If no original command has been supplied or the supplied message is
     * not a response or it has the wrong exchange ID or is not the
     * expected type of response, abort with ARCP_ERROR_INTERNAL.
     */
    if (resp_msg->header.msg_type!=ARCP_MSG_RESPONSE ||
        resp_msg->header.exchange_id!=orig_cmd->header.exchange_id ||
        resp_msg->response.id!=resp_id) {
      return ARCP_ERROR_INTERNAL;
    }

    resp_to_send = resp_msg;
  }
  resp_to_send->header.exchange_id = orig_cmd->header.exchange_id;

  /* Override the connection's ARCP version if the incoming command is
   * from an earlier version of the protocol.
   */
  if (orig_cmd->header.protocol_version < handle->connection_arcp_version)
    handle->connection_arcp_version = orig_cmd->header.protocol_version;

  /* Send the response message */
  err = arcp_msg_write(handle,resp_to_send);

  /* Free the response message if it was not supplied in pre-prepared form */
  if (resp_msg == NULL)
    arcp_msg_free(resp_to_send);

  return err;
}
/* ======================================================================== */

static signed int arcp_do_get_sys_info(arcp_handle_t *handle, arcp_cmd_id_t cmd_id,
  arcp_resp_id_t expected_resp_id, arcp_msg_t **resp_ret) {
/*
 * Internal function.  Sends one of the ARCP_CMD_GET_SYS* commands (given in
 * cmd_id) to a slave via the given ARCP handle (handle).  If no error is
 * detected and the correct response packet (expected_resp_id) is received
 * the response packet is returned so long as the caller has not supplied
 * NULL as the last parameter.  Otherwise an error code is returned and the
 * *resp_ret is set to NULL.
 */
arcp_msg_t *resp;
signed int res = arcp_exec_cmd(handle, cmd_id, NULL, &resp);

  if (resp_ret != NULL)
    *resp_ret = NULL;

  /* Ensure a valid response was received.  No requests for system info
   * should ever return a simple ACK.
   */
  if (arcp_id_is_response(res) && res!=expected_resp_id && res!=ARCP_RESP_NAK &&
      res!=ARCP_RESP_UNK) {
    res = ARCP_ERROR_BAD_RESPONSE;
  }

  /* Return the resulting message, but only if there is no error */
  if (resp_ret!=NULL && res==expected_resp_id)
    *resp_ret = resp;
  else
    arcp_msg_free(resp);

  return res;
}
/* ======================================================================== */

static signed int arcp_do_set_params(arcp_handle_t *handle, arcp_cmd_id_t cmd_id, 
  arcp_msg_t *msg) {
/*
 * Internal function.  Attempts to set the parameters of the slave via the
 * given ARCP handle (handle).  It does this by passing the previously
 * prepared ARCP message (msg) to the slave, reading the response and sanity
 * checking it before returning.
 *
 * Return value will be an ARCP_RESP_* response code or an ARCP_ERROR_*
 * error code, indicating the success or otherwise of the attempt.  If the
 * incorrect type of message has been provided, ARCP_ERROR_INTERNAL is
 * returned.
 *
 * The provision of cmd_id is of course not technically required, but
 * it allows this function to explicitly ensure that the message being
 * passed matches the programmer's intent.
 */

signed int res;
arcp_msg_t *resp = NULL;

  /* Ensure the message is of the correct type */
  if (msg->header.msg_type!=ARCP_MSG_COMMAND || msg->command.id!=cmd_id) {
    return ARCP_ERROR_INTERNAL;
  }

  /* Execute the command */
  res = arcp_exec_cmd(handle, cmd_id, msg, &resp);

  /* Ensure a valid response was received.  When setting parameters the only
   * valid responses are simple responses.  Don't however overwrite any
   * error codes (res<0) which might have been returned.  In addition, if an
   * error code was returned in a NAK, return it instead of the NAK response
   * ID.
   */
  if (res == ARCP_RESP_NAK) {
    if (resp->response.info_code < 0)
      res = resp->response.info_code;
  } else
  if (arcp_id_is_response(res) && res!=ARCP_RESP_ACK && res!=ARCP_RESP_UNK)
    res = ARCP_ERROR_BAD_RESPONSE;

  arcp_msg_free(resp);
  return res;
}
/* ======================================================================== */

uint32 arcp_get_lib_version(void) {
/*
 * Returns the version of the libarcp library in a 32 bit integer.  This
 * comprises a major, minor and build triple.  Major/minor together describe
 * the ARCP protocol version implemented by the library while build allows
 * for bugfixes and other alterations which do not by themselves constitute
 * an increase in the ARCP protocol number.
 *
 * If byte 0 is taken as the least significant byte, the major version
 * number will be in byte 2, the minor version number will appear in byte 1
 * and the "build" version will be in byte 0.  Byte 3 is currently always
 * zero.
 */
  return LIBARCP_VERSION_WORD(LIBARCP_VERSION_MAJOR, LIBARCP_VERSION_MINOR,
    LIBARCP_VERSION_BUILD);
}
/* ======================================================================== */

unsigned int arcp_get_lib_proto_version(void) {
/*
 * Return the (highest) ARCP protocol version supported by this library. 
 * The least significant byte of the return value will contain the minor
 * version number while the major version number will be in the next
 * significant byte.  The two higher-order bytes are currently always
 * zero.
 */
  return ARCP_VERSION_WORD(ARCP_VERSION_MAJOR, ARCP_VERSION_MINOR);
}
/* ======================================================================== */

signed int arcp_reset(arcp_handle_t *handle) {
/*
 * Sends an ARCP reset packet to the given ARCP handle and processes the
 * response.  For full details on the working of this function refer to
 * arcp_do_simple_cmd().
 */
signed int res = arcp_exec_cmd(handle, ARCP_CMD_RESET, NULL, NULL);

  /* Ensure a valid reset response was received.  All ARCP slaves should
   * accept the RESET command, so ARCP_RESP_UNK is not a valid response.
   */
  if (arcp_id_is_response(res) && res!=ARCP_RESP_NAK && res!=ARCP_RESP_ACK) 
    return ARCP_ERROR_BAD_RESPONSE;
  return res;
}
/* ======================================================================== */

signed int arcp_ping(arcp_handle_t *handle) {
/*
 * Sends an ARCP ping packet to the given ARCP handle and processes the
 * response.  For full details on the working of this function refer to
 * arcp_do_simple_cmd().
 */
signed int res = arcp_exec_cmd(handle, ARCP_CMD_PING, NULL, NULL);

  /* Ensure a valid PING response was received.  The only valid response from
   * a PING command is an ACK.
   */
  if (arcp_id_is_response(res) && res!=ARCP_RESP_ACK) 
    return ARCP_ERROR_BAD_RESPONSE;
  return res;
}
/* ======================================================================== */

signed int arcp_get_sysid(arcp_handle_t *handle, arcp_sysid_t **sysid) {
/*
 * Sends an ARCP "get sysid" packet to the ARCP handle and processes
 * the response.  If no error is detected the response packet is returned
 * so long as the caller has not supplied NULL as the last parameter.
 *
 * Return value is 0 on success, or an ARCP_ERROR_* code on failure.
 */
arcp_msg_t *ret;
signed int i;

  i = arcp_do_get_sys_info(handle, ARCP_CMD_GET_SYSID, ARCP_RESP_SYSID, &ret);
  if (i==ARCP_RESP_SYSID && ret!=NULL && sysid!=NULL) {
    *sysid = ret->resp_sysid.sysid;
    /* Set this to NULL so arcp_msg_free() doesn't dispose of it; freeing
     * it would be bad since it's being returned to the caller.
     */
    ret->resp_sysid.sysid = NULL;
    i = ARCP_RESP_ACK;
  } else
    *sysid = NULL;

  arcp_msg_free(ret);
  return i;
}
/* ======================================================================== */

signed int arcp_get_sysstat(arcp_handle_t *handle, arcp_sysstat_t **sysstat) {
/*
 * Sends an ARCP "get sysstat" packet to the ARCP handle and processes
 * the response.  If no error is detected the response packet is returned
 * so long as the caller has not supplied NULL as the last parameter.
 *
 * Return value is 0 on success, or an ARCP_ERROR_* code on failure.
 */
arcp_msg_t *ret;
signed int i;

  i = arcp_do_get_sys_info(handle, ARCP_CMD_GET_SYSSTAT,ARCP_RESP_SYSSTAT,&ret);
  if (i==ARCP_RESP_SYSSTAT && ret!=NULL && sysstat!=NULL) {
    *sysstat = ret->resp_sysstat.sysstat;
    ret->resp_sysstat.sysstat = NULL;
    i = ARCP_RESP_ACK;
  } else
    *sysstat = NULL;

  arcp_msg_free(ret);
  return i;
}
/* ======================================================================== */

signed int arcp_set_module_enable(arcp_handle_t *handle, uint8 enable) {
/*
 * Sets the enable status of the module connected through the given ARCP
 * handle to that given.
 */

arcp_msg_t *msg = arcp_msg_new(ARCP_MSG_COMMAND);
signed int res;

  /* Prepare the message for transmission */
  if (msg == NULL)
    return ARCP_ERROR_LOCAL;
  msg->command.id = ARCP_CMD_SET_MODULE_ENABLE;
  msg->cmd_enable.enable = enable;

  res = arcp_exec_cmd(handle, ARCP_CMD_SET_MODULE_ENABLE, msg, NULL);
  arcp_msg_free(msg);
  
  /* Note that all ARCP nodes should know about the SET_MODULE_ENABLE
   * command, so an "unknown command" response is not considered valid.
   */
  if (arcp_id_is_response(res) && res!=ARCP_RESP_ACK && res!=ARCP_RESP_NAK)
    return ARCP_ERROR_BAD_RESPONSE;
  return res;
}
/* ======================================================================== */

signed int arcp_set_pulseparam(arcp_handle_t *handle, uint8 slot, 
  arcp_pulse_t *param) {
/*
 * Sends an ARCP message to the slave connected to ARCP handle "handle" to
 * request the setting of the specified slot to the given pulse parameters. 
 * The result of the request is returned.  If there is insufficient memory
 * to process the request, ARCP_ERROR_LOCAL is returned.
 */
arcp_msg_t *msg = arcp_msg_new(ARCP_MSG_COMMAND);
signed int i;
  if (msg == NULL)
    return ARCP_ERROR_LOCAL;
  if (arcp_msg_set_cmd_id(msg, ARCP_CMD_SET_PULSE_PARAM) < 0) {
    arcp_msg_free(msg);
    return ARCP_ERROR_LOCAL;
  }

  msg->cmd_set_pulse_param.pulse_map_index = slot;
  msg->cmd_set_pulse_param.pulse_param = *param;
  i = arcp_do_set_params(handle, ARCP_CMD_SET_PULSE_PARAM, msg);

  /* Don't automatically deallocate the pulse code since the caller supplied
   * it.
   */
  msg->cmd_set_pulse_param.pulse_param.code = NULL;
  arcp_msg_free(msg);

  return i;
}
/* ======================================================================== */

signed int arcp_set_pulseseq(arcp_handle_t *handle, arcp_pulseseq_t *seq) {
/*
 * Sends an ARCP message to the slave connected to ARCP handle "handle"
 * to request the setting of the pulse sequence.  The result of the request
 * is returned.
 */
arcp_msg_t *msg = arcp_msg_new(ARCP_MSG_COMMAND);
signed int i;
  if (msg == NULL)
    return ARCP_ERROR_LOCAL;
  if (arcp_msg_set_cmd_id(msg, ARCP_CMD_SET_PULSE_SEQ) < 0) {
    arcp_msg_free(msg);
    return ARCP_ERROR_LOCAL;
  }

  msg->cmd_set_pulse_seq.seq = seq;
  i = arcp_do_set_params(handle, ARCP_CMD_SET_PULSE_SEQ, msg);

  /* Don't automatically deallocate the pulse sequence since the caller
   * supplied it.
   */
  msg->cmd_set_pulse_seq.seq = NULL;
  arcp_msg_free(msg);

  return i;
}
/* ======================================================================== */

signed int arcp_set_pulseseq_index(arcp_handle_t *handle, uint16 index) {
/*
 * Sends an ARCP message to the slave connected to ARCP handle "handle" to
 * request the setting of the pulse sequence index.  The result of the
 * request is returned.
 */
arcp_msg_t *msg = arcp_msg_new(ARCP_MSG_COMMAND);
signed int res;

  /* Prepare the message for transmission */
  if (msg == NULL)
    return ARCP_ERROR_LOCAL;
  if (arcp_msg_set_cmd_id(msg, ARCP_CMD_SET_PULSE_SEQ_IDX) < 0) {
    arcp_msg_free(msg);
    return ARCP_ERROR_LOCAL;
  }
  msg->cmd_set_pulse_seq_idx.seq_index = index;

  res = arcp_exec_cmd(handle, ARCP_CMD_SET_PULSE_SEQ_IDX, msg, NULL);
  arcp_msg_free(msg);
  
  if (arcp_id_is_response(res) && res!=ARCP_RESP_ACK && res!=ARCP_RESP_NAK &&
      res!=ARCP_RESP_UNK)
    return ARCP_ERROR_BAD_RESPONSE;
  return res;
}
/* ======================================================================== */

signed int arcp_set_trigparam(arcp_handle_t *handle, arcp_trigger_t *param) {
/*
 * Sends an ARCP message to the slave connected to ARCP handle "handle"
 * to request the setting of trigger parameters.  The result of the request
 * is returned.
 */
arcp_msg_t *msg = arcp_msg_new(ARCP_MSG_COMMAND);
signed int res;
  if (msg == NULL)
    return ARCP_ERROR_LOCAL;
  if (arcp_msg_set_cmd_id(msg, ARCP_CMD_SET_TRIG_PARAM) < 0) {
    arcp_msg_free(msg);
    return ARCP_ERROR_LOCAL;
  }
  msg->cmd_set_trig_param.trig_param = *param;
  res = arcp_do_set_params(handle, ARCP_CMD_SET_TRIG_PARAM, msg);

  arcp_msg_free(msg);
  return res;
}
/* ======================================================================== */

signed int arcp_set_usrctl_enable(arcp_handle_t *handle, uint8 enable) {
/*
 * Sets the enable status of the user controls on the module connected to
 * the ARCP handle "handle".  Note that not all modules have user controls.
 */

arcp_msg_t *msg = arcp_msg_new(ARCP_MSG_COMMAND);
signed int res;

  /* Prepare the message for transmission */
  if (msg == NULL)
    return ARCP_ERROR_LOCAL;
  msg->command.id = ARCP_CMD_SET_USRCTL_ENABLE;
  msg->cmd_usrctl_enable.enable = enable;

  res = arcp_exec_cmd(handle, ARCP_CMD_SET_USRCTL_ENABLE, msg, NULL);
  arcp_msg_free(msg);
  
  return res;
}
/* ======================================================================== */

signed int arcp_set_phase(arcp_handle_t *handle, uint16 phase_slot, arcp_phase_entry_t *phases, uint16 n_phases) {
/*
 * Sends a "set phase" command to the ARCP handle "handle".  The phases 
 * requested are passed in the "phases" parameter.  n_phases must be the
 * number of phase entries contained in "phases".
 *
 * This function does NOT free the phase entry list; doing so is a
 * responsibility of the caller.
 */
arcp_msg_t *msg = arcp_msg_new(ARCP_MSG_COMMAND);
signed int res;

  /* The "set phase" command is applicable only to protocol versions >= 1.1 */
  if (handle->connection_arcp_version < ARCP_VERSION_1_1) {
    return ARCP_RESP_UNK;
  }

  if (msg == NULL) {
    return ARCP_ERROR_LOCAL;
  }
  msg->command.id = ARCP_CMD_SET_PHASE;
  msg->cmd_set_phase.phase_slot = phase_slot;
  msg->cmd_set_phase.n_phases = n_phases;
  msg->cmd_set_phase.phases = phases;
  
  res = arcp_exec_cmd(handle, ARCP_CMD_SET_PHASE, msg, NULL);

  /* Don't automatically deallocate the phase table since the caller
   * supplied it.
   */
  msg->cmd_set_phase.phases = NULL;
  arcp_msg_free(msg);

  return res;
}
/* ======================================================================== */

signed int arcp_send_ack(arcp_handle_t *handle, arcp_msg_t *cmd_msg) {
/*
 * Sends an ACK message in response to the given command message.  The
 * return value is 0 on success or an ARCP_ERROR_* code.
 */
  return arcp_exec_resp(handle, cmd_msg, ARCP_RESP_ACK, NULL, 0);
}
/* ======================================================================== */

signed int arcp_send_nak(arcp_handle_t *handle, arcp_msg_t *cmd_msg, int16 err_code) {
/*
 * Sends a NAK message in response to the given command message.  The return
 * value is 0 on success or an ARCP_ERROR_* code.  "code" is additional
 * error information that the caller wishes to include in the NAK message.
 */
  return arcp_exec_resp(handle, cmd_msg, ARCP_RESP_NAK, NULL, err_code);
}
/* ======================================================================== */

signed int arcp_send_unk(arcp_handle_t *handle, arcp_msg_t *cmd_msg) {
/*
 * Sends an UNK (unknown command) message in response to the given command
 * message.  The return value is 0 on success or an ARCP_ERROR_* code.
 */
  return arcp_exec_resp(handle, cmd_msg, ARCP_RESP_UNK, NULL, 0);
}
/* ======================================================================== */

signed int arcp_send_sysid(arcp_handle_t *handle, arcp_msg_t *cmd_msg, 
  arcp_sysid_t *sysid) {
/*
 * Sends a sysid response message in answer to the supplied cmd_msg.
 */
arcp_msg_t *msg;
signed int i;

  if (cmd_msg==NULL || cmd_msg->header.msg_type!=ARCP_MSG_COMMAND || cmd_msg->command.id!=ARCP_CMD_GET_SYSID)
    return ARCP_ERROR_INTERNAL;

  msg = arcp_msg_new(ARCP_MSG_RESPONSE);
  if (msg == NULL)
    return ARCP_ERROR_LOCAL;

  arcp_msg_set_resp_id(msg, ARCP_RESP_SYSID);
  msg->header.exchange_id = cmd_msg->header.exchange_id;
  msg->resp_sysid.sysid = sysid;

  i = arcp_exec_resp(handle, cmd_msg, ARCP_RESP_SYSID, msg, 0);
  msg->resp_sysid.sysid = NULL;
  arcp_msg_free(msg);
  return i;
}
/* ======================================================================== */

signed int arcp_send_sysstat(arcp_handle_t *handle, arcp_msg_t *cmd_msg, 
  arcp_sysstat_t *sysstat) {
/*
 * Sends a sysstat response message in answer to the supplied cmd_msg.
 */
arcp_msg_t *msg;
signed int i;
  if (cmd_msg==NULL || cmd_msg->header.msg_type!=ARCP_MSG_COMMAND || cmd_msg->command.id!=ARCP_CMD_GET_SYSSTAT)
    return ARCP_ERROR_INTERNAL;
  msg = arcp_msg_new(ARCP_MSG_RESPONSE);
  if (msg == NULL)
    return ARCP_ERROR_LOCAL;

  arcp_msg_set_resp_id(msg, ARCP_RESP_SYSSTAT);
  msg->header.exchange_id = cmd_msg->header.exchange_id;
  msg->resp_sysstat.sysstat = sysstat;

  i = arcp_exec_resp(handle, cmd_msg, ARCP_RESP_SYSSTAT, msg, 0);
  msg->resp_sysstat.sysstat = NULL;
  arcp_msg_free(msg);
  return i;
}
/* ======================================================================== */
