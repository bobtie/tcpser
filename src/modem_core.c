#include "getcmd.h"
#include "debug.h"
#include "modem_core.h"

char* mdm_responses[37];

int mdm_init() {
  mdm_responses[MDM_RESP_OK]="OK";
  mdm_responses[MDM_RESP_RING]="RING";
  mdm_responses[MDM_RESP_ERROR]="ERROR";
  mdm_responses[MDM_RESP_CONNECT]="CONNECT";
  mdm_responses[MDM_RESP_NO_CARRIER]="NO CARRIER";
  mdm_responses[MDM_RESP_CONNECT_1200] = "CONNECT 1200";
  mdm_responses[MDM_RESP_NO_DIALTONE] = "NO DIALTONE";
  mdm_responses[MDM_RESP_BUSY] = "BUSY";
  mdm_responses[MDM_RESP_NO_ANSWER] = "NO ANSWER";
  mdm_responses[MDM_RESP_CONNECT_0600] = "CONNECT 0600";
  mdm_responses[MDM_RESP_CONNECT_2400] = "CONNECT 2400";
  mdm_responses[MDM_RESP_CONNECT_4800] = "CONNECT 4800";
  mdm_responses[MDM_RESP_CONNECT_9600] = "CONNECT 9600";
  mdm_responses[MDM_RESP_CONNECT_7200] = "CONNECT 7200";
  mdm_responses[MDM_RESP_CONNECT_12000] = "CONNECT 12000";
  mdm_responses[MDM_RESP_CONNECT_14400] = "CONNECT 14400";
  mdm_responses[MDM_RESP_CONNECT_19200] = "CONNECT 19200";
  mdm_responses[MDM_RESP_CONNECT_38400] = "CONNECT 38400";
  mdm_responses[MDM_RESP_CONNECT_57600] = "CONNECT 57600";
  mdm_responses[MDM_RESP_CONNECT_115200] = "CONNECT 115200";
  mdm_responses[MDM_RESP_CONNECT_234000]="CONNECT 230400";
  return 0;
}


int get_connect_response(int speed, int level) {
  if(level == 0) {
    return MDM_RESP_CONNECT;
  }
  switch (speed) {
    case 115200:
      return MDM_RESP_CONNECT_115200;
    case 57600:
      return MDM_RESP_CONNECT_57600;
    case 38400:
      return MDM_RESP_CONNECT_38400;
    case 19200:
      return MDM_RESP_CONNECT_19200;
    case 9600:
      return MDM_RESP_CONNECT_9600;
    case 4800:
      return MDM_RESP_CONNECT_4800;
    case 2400:
      return MDM_RESP_CONNECT_2400;
    case 1200:
      return MDM_RESP_CONNECT_1200;
    case 600:
      return MDM_RESP_CONNECT_0600;
  }
  return MDM_RESP_CONNECT;
}

void mdm_init_config(modem_config* cfg) {
  int i=0;
  cfg->send_responses=TRUE;
  cfg->connect_response=0;
  cfg->response_code_level=4;
  cfg->text_responses=TRUE;
  cfg->echo=TRUE;
  cfg->cmd_mode=TRUE;
  cfg->connected=FALSE;
  cfg->off_hook=FALSE;
  cfg->line_ringing=FALSE;
  cfg->cur_line_idx=0;

  for(i=0;i<100;i++) {
    cfg->s[i]=0;
  }
  cfg->s[2]=43;
  cfg->s[3]=13;
  cfg->s[4]=10;
  cfg->s[5]=8;
  cfg->s[6]=2;
  cfg->s[7]=50;
  cfg->s[8]=2;
  cfg->s[9]=6;
  cfg->s[10]=14;
  cfg->s[11]=95;
  cfg->s[12]=50;

  cfg->crlf[0]=cfg->s[3];
  cfg->crlf[1]=cfg->s[4];
  cfg->crlf[2]=0;

  cfg->last_char='\0';
  cfg->dial_type=0;
  cfg->last_dial_type=0;
  cfg->disconnect_delay=0;

  cfg->pre_break_delay=FALSE;
  cfg->break_len=0;

  cfg->memory_dial=FALSE;
  cfg->dsr_active=FALSE;
  cfg->dsr_on=TRUE;
  cfg->dcd_on=FALSE;
  cfg->found_a=FALSE;
  cfg->cmd_started=FALSE;
  cfg->allow_transmit=TRUE;
  cfg->invert_dsr=FALSE;
  cfg->invert_dcd=FALSE;

  cfg->config0[0]='\0';
  cfg->config1[0]='\0';

  dce_init_config(cfg);
  sh_init_config(cfg);
  line_init_config(cfg);
}

int get_new_cts_state(modem_config *cfg, int up) {
  return MDM_CL_CTS_HIGH;
}


int get_new_dsr_state(modem_config *cfg, int up) {
  if(cfg->dsr_on == TRUE)
    return (cfg->invert_dsr == TRUE?MDM_CL_DSR_LOW:MDM_CL_DSR_HIGH);
  if((up == TRUE && cfg->invert_dsr == FALSE) 
     || (up == FALSE && cfg->invert_dsr==TRUE)
    )
    return MDM_CL_DSR_HIGH;
  else
    return MDM_CL_DSR_LOW;
}


int get_new_dcd_state(modem_config *cfg, int up) {
  if(cfg->dcd_on == TRUE)
    return (cfg->invert_dcd == TRUE?MDM_CL_DCD_LOW:MDM_CL_DCD_HIGH);
  if((up == TRUE && cfg->invert_dcd == FALSE) 
     || (up == FALSE && cfg->invert_dcd==TRUE)
    )
    return MDM_CL_DCD_HIGH;
  else
    return MDM_CL_DCD_LOW;
}

int mdm_set_control_lines(modem_config *cfg) {
  int state=0;
  int up=cfg->connected;

  state |=get_new_cts_state(cfg,up);
  state |=get_new_dsr_state(cfg,up);
  state |=get_new_dcd_state(cfg,up);

  LOG(LOG_INFO,"Control Lines: DSR:%d DCD:%d CTS:%d",
      (state & MDM_CL_DSR_HIGH),
      (state & MDM_CL_DCD_HIGH),
      (state & MDM_CL_CTS_HIGH)
     );

  dce_set_control_lines(cfg,state);
}


void mdm_write_char(modem_config *cfg,char data) {
  char str[2];
  str[0]=data;
  mdm_write(cfg,str,1);
}

void mdm_write(modem_config *cfg,char data[], int len) {
  if(cfg->allow_transmit == TRUE) {
    dce_write(cfg,data,len);
  }
}

void mdm_send_response(int msg,modem_config *cfg) {
  LOG(LOG_DEBUG,"Sending %s response to modem",mdm_responses[msg]);
  if(cfg->send_responses==TRUE) {
    mdm_write(cfg,cfg->crlf,2);
    if(cfg->text_responses==TRUE) {
      LOG(LOG_ALL,"Sending text response");
      mdm_write(cfg,mdm_responses[msg],strlen(mdm_responses[msg]));
    } else {
      LOG(LOG_ALL,"Sending numeric response");
      char msgID[17];
      sprintf(msgID,"%d",msg);
      mdm_write(cfg,msgID,strlen(msgID));
    }
    mdm_write(cfg,cfg->crlf,2);
  }
}

int mdm_off_hook(modem_config *cfg) {
  int speed;

  LOG(LOG_INFO,"taking modem off hook");
  cfg->off_hook=TRUE;
  cfg->cmd_mode=FALSE;
  line_off_hook(cfg);
  if(cfg->line_ringing == TRUE) {
    cfg->connected=TRUE;
    mdm_set_control_lines(cfg);
    switch(cfg->connect_response) {
      case 2:
        speed=cfg->dte_speed;
        break;
      default:
        speed=cfg->dce_speed;
        break;
  
    } 
    mdm_send_response(get_connect_response(speed,cfg->response_code_level),cfg);
    mdm_set_control_lines(cfg);
  }
  return 0;
}

int mdm_connect(modem_config* cfg) {
  int speed;
  mdm_off_hook(cfg);
  if(cfg->connected == FALSE) {
    if(line_connect(cfg) == 0) {
      cfg->connected=TRUE;
      mdm_set_control_lines(cfg);
      switch(cfg->connect_response) {
        case 2:
          speed=cfg->dte_speed;
          break;
        default:
          speed=cfg->dce_speed;
          break;

      } 
      mdm_send_response(get_connect_response(speed,cfg->response_code_level),cfg);
    } else {
      cfg->connected=TRUE;   // so disconnect will print NO CARRIER
      mdm_disconnect(cfg);
    }
  }
  return 0;
}


int mdm_listen(modem_config *cfg) {
  line_listen(cfg);
}


int mdm_disconnect(modem_config* cfg) {
  LOG_ENTER();
  LOG(LOG_INFO,"Disconnecting modem");
  int up=cfg->connected;
  cfg->connected=FALSE;
  cfg->off_hook=FALSE;
  cfg->cmd_mode=TRUE;
  cfg->break_len=0;
  cfg->line_ringing = FALSE;
  cfg->pre_break_delay=FALSE;
  mdm_set_control_lines(cfg);
  line_disconnect(cfg);
  if(up == TRUE) {
    mdm_send_response(MDM_RESP_NO_CARRIER,cfg);
    usleep(cfg->disconnect_delay * 1000);
  }
  cfg->rings=0;
  mdm_listen(cfg);
  LOG_EXIT();
  return 0;
}

int config_modem(modem_config* cfg) {
  int done=FALSE;
  int index=0;
  int num=0;
  int start=0;
  int end=0;
  int cmd=AT_CMD_NONE;
  char* command=cfg->cur_line;
  char tmp[256];

  LOG_ENTER();

  LOG(LOG_DEBUG,"Evaluating AT%s",command);

  while(TRUE != done ) {
    if(cmd != AT_CMD_ERR) {
      cmd=getcmd(command,&index,&num, &start,&end);
          LOG(LOG_DEBUG,"Command: %c, Flags: %d, index=%d, num=%d, data=%d-%d",
                 cmd,
                 cmd /256,
                 index,
                 num,
                 start,
                 end
                );
    }
    switch(cmd) {
      case AT_CMD_ERR:
        mdm_send_response(MDM_RESP_ERROR,cfg);
        done=TRUE;
        break;
      case AT_CMD_END:
        if(cfg->cmd_mode == TRUE)
          mdm_send_response(MDM_RESP_OK,cfg);
        done=TRUE;
        break;
      case AT_CMD_NONE:
        done=TRUE;
        break;
      case 'O':
      case 'A':
          mdm_off_hook(cfg);
          cmd=AT_CMD_END;
          done=TRUE;
          break;
      case 'B':   // 212A versus V.22 connection
        if(num > 1) {
          cmd-AT_CMD_ERR;
        } else {
          //cfg->connect_1200=num;
        }
        break;
      case 'D':
        if(end>start) {
          strncpy(cfg->dialno,command+start,end-start);
          cfg->dialno[end-start]='\0';
          cfg->dial_type=(char)num;
          cfg->last_dial_type=(char)num;
          strncpy(cfg->last_dialno,command+start,end-start);
          cfg->last_dialno[end-start]='\0';
          cfg->memory_dial=FALSE;
        } else if (num == 'L') {
          strncpy(cfg->dialno,cfg->last_dialno,strlen(cfg->last_dialno));
          cfg->dial_type=cfg->dial_type;
          cfg->memory_dial=TRUE;
          mdm_write(cfg,cfg->crlf,2);
          mdm_write(cfg,cfg->dialno,strlen(cfg->dialno));
        } else {
          cfg->dialno[0]=0;
          cfg->last_dialno[0]=0;
          cfg->dial_type=0;
          cfg->last_dial_type=0;
        }
        if(strlen(cfg->dialno) > 0)
          mdm_connect(cfg);
        else
          mdm_off_hook(cfg);
        done=TRUE;
        break;
      case 'E':   // still need to define #2
        if(num == 0)
          cfg->echo=FALSE;
        else if(num == 1)
          cfg->echo=TRUE;
        else {
          cmd=AT_CMD_ERR;
        }
        break;
      case 'H':
          if(num == 0) {
            mdm_disconnect(cfg);
          } else if(num == 1) {
            mdm_off_hook(cfg);
          } else
            cmd=AT_CMD_ERR;
          break;
      case 'I':   // Information.
        break;
      case 'L':   // Speaker volume
        if(num < 1 || num > 3)
          cmd-AT_CMD_ERR;
        else {
          //cfg->volume=num;
        }
        break;
      case 'M':   // speaker settings
        if(num > 3)
          cmd-AT_CMD_ERR;
        else {
          //cfg->speaker_setting=num;
        }
        break;
      case 'N':   // automode negotiate
        if(num > 1)
          cmd-AT_CMD_ERR;
        else {
          //cfg->auto_mode=num;
        }
        break;
      case 'P':   // defaut to pulse dialing
        //cfg->default_dial_type=MDM_DT_PULSE;
        break;
      case 'Q':   // still need to define #2
        if(num == 0)
          cfg->send_responses=FALSE;
        else if(num == 1)
          cfg->send_responses=TRUE;
        else if(num == 2)  // this should be yes orig/no answer.
          cfg->send_responses=TRUE;
        else {
          cmd=AT_CMD_ERR;
        }
        break;
      case 'S':
          strncpy(tmp,command+start,end-start);
          tmp[end-start]='\0';
          cfg->s[num]=atoi(tmp);
          switch(num) {
            case 3:
              cfg->crlf[0]=cfg->s[3];
              break;
            case 4:
              cfg->crlf[1]=cfg->s[4];
              break;
          }
          break;
      case 'T':   // defaut to tone dialing
        //cfg->default_dial_type=MDM_DT_TONE;
        break;
      case 'V':   // done
        if(num == 0)
          cfg->text_responses=FALSE;
        else if(num ==1)
          cfg->text_responses=TRUE;
        else {
          cmd=AT_CMD_ERR;
        }
        break;
      case 'W':
          if(num > -1 && num < 3) 
            cfg->connect_response=num;
          else
            cmd=AT_CMD_ERR;
          break;
      case 'X':
          if(num > -1 && num < 5) 
            cfg->response_code_level=num;
          else
            cmd=AT_CMD_ERR;
          break;
      case 'Y':   // long space disconnect.
        if(num > 1)
          cmd-AT_CMD_ERR;
        else {
          //cfg->long_disconnect=num;
        }
        break;
      case 'Z':   // long space disconnect.
        if(num > 1)
          cmd-AT_CMD_ERR;
        else {
          // set config0 to cur_line and go.
        }
        break;
      case AT_CMD_FLAG_EXT + 'K':
        // flow control.
        switch (num) {
          case 0:
            dce_set_flow_control(cfg,0);
            break;
          case 3:
            dce_set_flow_control(cfg,MDM_FC_RTS);
            break;
          case 4:
            dce_set_flow_control(cfg,MDM_FC_XON);
            break;
          case 5:
            dce_set_flow_control(cfg,MDM_FC_XON);
            // need to add passthrough..  Not sure how.
            break;
          case 6:
            dce_set_flow_control(cfg,MDM_FC_XON | MDM_FC_RTS);
            break;
          default:
            cmd=AT_CMD_ERR;
            break;
        }
        break;
      default:
        break;
    }
  } 
  cfg->cur_line_idx=0;
  return cmd;
}

int mdm_handle_char(modem_config* cfg, char ch) {
  if(cfg->echo == TRUE)
    mdm_write_char(cfg,ch);
  if(cfg->cmd_started == TRUE) {
    if(ch == (char)(cfg->s[5])) {
      if(cfg->cur_line_idx == 0 && cfg->echo == TRUE) {
        mdm_write_char(cfg,'T');
      } else {
        cfg->cur_line_idx--;
      }
    } else if(ch == (char)(cfg->s[3])) {
      // we have a line, process.
      cfg->cur_line[cfg->cur_line_idx]=0;
      config_modem(cfg);
      cfg->cur_line_idx=0;
      cfg->cmd_started=FALSE;
    } else {
      cfg->cur_line[cfg->cur_line_idx++ % sizeof(cfg->cur_line)]=ch;
    }
  } else if(cfg->found_a == TRUE) {
    cfg->found_a=FALSE;
    if(ch == 't' || ch == 'T') {
      cfg->cmd_started=TRUE;
      LOG(LOG_ALL,"We found a 'T' in the serial stream, switching to command parse mode");
    }
  } else if(ch == 'a' || ch == 'A') {
    LOG(LOG_ALL,"We found an 'A' in the serial stream");
    cfg->found_a=TRUE;
  } 
}

int mdm_clear_break(modem_config* cfg) {
  cfg->break_len=0;
  cfg->pre_break_delay=FALSE;
}

int mdm_parse_data(modem_config* cfg,char* data, int len) {
  int i;

  if(cfg->cmd_mode==TRUE) {
    for(i=0;i<len;i++) {
      mdm_handle_char(cfg,data[i]);
    }
  } else {
    line_write(cfg,data,len);
    if(cfg->pre_break_delay == TRUE) {
      for(i=0;i<len;i++) {
        if(data[i] == (char)cfg->s[2]) {
          LOG(LOG_DEBUG,"Break character received");
          cfg->break_len++;
          if(cfg->break_len > 3) {  // more than 3, considered invalid
            cfg->pre_break_delay=FALSE;
            cfg->break_len=0;
          }
        } else {
          LOG(LOG_ALL,"Found non-break character, cancelling break");
          // chars past +++
          mdm_clear_break(cfg);
        }
      }
    }
  }
  return 0;
}

int mdm_handle_timeout(modem_config* cfg) {
  if(cfg->pre_break_delay == TRUE && cfg->break_len == 3) {
    // pre and post break.
    LOG(LOG_INFO,"Break condition detected");
    cfg->cmd_mode=TRUE;
    mdm_send_response(MDM_RESP_OK,cfg);
    mdm_clear_break(cfg);
  } else if(cfg->pre_break_delay == FALSE) {
    // pre break wait over.
    LOG(LOG_DEBUG,"Initial Break Delay detected");
    cfg->pre_break_delay=TRUE;
  } else if(cfg->pre_break_delay == TRUE 
            && cfg->break_len > 0
           ) {
    LOG(LOG_ALL,"Inter-break-char delay time exceeded");
    mdm_clear_break(cfg);
  } else if(cfg->s[30] != 0) {
    // timeout...
    LOG(LOG_INFO,"DTE communication inactivity timeout");
    mdm_disconnect(cfg);
  }
}

int mdm_send_ring(modem_config *cfg) {
  LOG(LOG_DEBUG,"Sending 'RING' to modem");
  cfg->line_ringing = TRUE;
  mdm_send_response(MDM_RESP_RING,cfg);
  cfg->rings++;
  LOG(LOG_ALL,"Sent #%d ring",cfg->rings);
  if(cfg->cmd_mode == FALSE || (cfg->s[0] != 0 &&cfg->rings>=cfg->s[0])) {
    mdm_off_hook(cfg);
  }
  return 0;
}

