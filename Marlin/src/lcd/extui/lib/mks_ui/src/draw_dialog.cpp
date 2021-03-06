#include "../../../../../MarlinCore.h"
#if ENABLED(TFT_LITTLE_VGL_UI)
#include "lv_conf.h"
//#include "../../lvgl/src/lv_objx/lv_imgbtn.h"
//#include "../../lvgl/src/lv_objx/lv_img.h"
//#include "../../lvgl/src/lv_core/lv_disp.h"
//#include "../../lvgl/src/lv_core/lv_refr.h"
//#include "../../MarlinCore.h"
#include "../inc/draw_ui.h"
#include "../../../../../sd/cardreader.h"
#include "../../../../../gcode/queue.h"
#include "../../../../../module/temperature.h"
#include "../../../../../module/planner.h"
#if ENABLED(POWER_LOSS_RECOVERY)
#include "../../../../../feature/powerloss.h"
#endif
#if ENABLED(PARK_HEAD_ON_PAUSE)
#include "../../../../../feature/pause.h"
#endif
#include "../../../../../gcode/gcode.h"

static lv_obj_t * scr;
extern uint8_t sel_id;
extern uint8_t once_flag;
extern uint8_t gcode_preview_over;
extern int upload_result ;
extern uint32_t upload_time;
extern uint32_t upload_size;

static void btn_ok_event_cb(lv_obj_t * btn, lv_event_t event)
{
    if(event == LV_EVENT_CLICKED) {
       
    }
    else if(event == LV_EVENT_RELEASED)
    {
    	if(uiCfg.dialogType == DIALOG_TYPE_PRINT_FILE)
    	{
		preview_gcode_prehandle(list_file.file_name[sel_id]);
		reset_print_time();
		start_print_time();
		
		uiCfg.print_state = WORKING;
		lv_clear_dialog();
		lv_draw_printing();
		
		if(gcode_preview_over != 1)
		{
			#if ENABLED (SDSUPPORT)
			char *cur_name;
			cur_name=strrchr(list_file.file_name[sel_id],'/');

			SdFile file;
			SdFile *curDir;
			card.endFilePrint();
			const char * const fname = card.diveToFile(true, curDir, cur_name);
			if (!fname) return;
			if (file.open(curDir, fname, O_READ))
			{
				gCfgItems.curFilesize = file.fileSize();
				file.close();
				update_spi_flash();
			}
			card.openFileRead(cur_name);
			if(card.isFileOpen())
			{
			    	    feedrate_percentage = 100;
	                        //saved_feedrate_percentage = feedrate_percentage;
	                        planner.flow_percentage[0] = 100;
	                        planner.e_factor[0]= planner.flow_percentage[0]*0.01;
	                        if(EXTRUDERS==2)
	                        {
	                            planner.flow_percentage[1] = 100;
	                            planner.e_factor[1]= planner.flow_percentage[1]*0.01;  
	                        }                            
				    card.startFileprint();
				    #if ENABLED(POWER_LOSS_RECOVERY)
      				recovery.prepare();
    				#endif
				    once_flag = 0;
			}
			#endif
		}
    	}
	else if(uiCfg.dialogType == DIALOG_TYPE_STOP)
	{
		stop_print_time();
		lv_clear_dialog();
		lv_draw_ready_print();

		#if ENABLED (SDSUPPORT)
		//card.endFilePrint();
		//wait_for_heatup = false;
		uiCfg.print_state = IDLE;
		card.flag.abort_sd_printing = true;
		//queue.clear();
        	//quickstop_stepper();
		//print_job_timer.stop();
		//thermalManager.disable_all_heaters();

		//#if ENABLED(POWER_LOSS_RECOVERY)
		//recovery.purge();
		//#endif
		//queue.enqueue_one_now(PSTR("G91"));
		//queue.enqueue_one_now(PSTR("G1 Z10"));
		//queue.enqueue_one_now(PSTR("G90"));
		//queue.enqueue_one_now(PSTR("G28 X0 Y0"));
		//queue.inject_P(PSTR("G91\nG1 Z10\nG90\nG28 X0 Y0\nM84\nM107"));
		#endif
	}
	else if(uiCfg.dialogType == DIALOG_TYPE_FINISH_PRINT)
	{
		clear_cur_ui();
		lv_draw_ready_print();
	}
	#if ENABLED(ADVANCED_PAUSE_FEATURE)
	else if(uiCfg.dialogType == DIALOG_PAUSE_MESSAGE_WAITING
		||uiCfg.dialogType == DIALOG_PAUSE_MESSAGE_INSERT
		||uiCfg.dialogType == DIALOG_PAUSE_MESSAGE_HEAT)
	{
		wait_for_user = false;
	}
	else if(uiCfg.dialogType == DIALOG_PAUSE_MESSAGE_OPTION)
	{
		pause_menu_response = PAUSE_RESPONSE_EXTRUDE_MORE;
	}
	else if(uiCfg.dialogType == DIALOG_PAUSE_MESSAGE_RESUME)
	{
		clear_cur_ui();
		draw_return_ui();
	}
	#endif
	else if(uiCfg.dialogType == DIALOG_STORE_EEPROM_TIPS)
	{
		gcode.process_subcommands_now_P(PSTR("M500"));
		clear_cur_ui();
		draw_return_ui();
	}
	else if(uiCfg.dialogType == DIALOG_READ_EEPROM_TIPS)
	{
		gcode.process_subcommands_now_P(PSTR("M501"));
		clear_cur_ui();
		draw_return_ui();
	}
	else if(uiCfg.dialogType == DIALOG_REVERT_EEPROM_TIPS)
	{
		gcode.process_subcommands_now_P(PSTR("M502"));
		clear_cur_ui();
		draw_return_ui();
	}
	else if(uiCfg.dialogType == DIALOG_WIFI_CONFIG_TIPS)
	{
		uiCfg.configWifi = 1;
		clear_cur_ui();
		draw_return_ui();
	}
    }
}

static void btn_cancel_event_cb(lv_obj_t * btn, lv_event_t event)
{
    if(event == LV_EVENT_CLICKED) {
       
    }
    else if(event == LV_EVENT_RELEASED)
    {
    	if(uiCfg.dialogType == DIALOG_PAUSE_MESSAGE_OPTION)
	{
		#if ENABLED(ADVANCED_PAUSE_FEATURE)
		pause_menu_response = PAUSE_RESPONSE_RESUME_PRINT;
		#endif
	}
	else
	{
    		clear_cur_ui();
		draw_return_ui();
	}
    }
}


void lv_draw_dialog(uint8_t type)
{

	if(disp_state_stack._disp_state[disp_state_stack._disp_index] != DIALOG_UI)
	{
		disp_state_stack._disp_index++;
		disp_state_stack._disp_state[disp_state_stack._disp_index] = DIALOG_UI;
	}
	disp_state = DIALOG_UI;

	uiCfg.dialogType = type;

	scr = lv_obj_create(NULL, NULL);

	
	lv_obj_set_style(scr, &tft_style_scr);
  lv_scr_load(scr);
  lv_obj_clean(scr);

  lv_obj_t * title = lv_label_create(scr, NULL);
	lv_obj_set_style(title, &tft_style_lable_rel);
	lv_obj_set_pos(title,TITLE_XPOS,TITLE_YPOS);
	lv_label_set_text(title, creat_title_text());
  
  lv_refr_now(lv_refr_get_disp_refreshing());
	
	//LV_IMG_DECLARE(bmp_pic);

	static lv_style_t style_btn_rel;                        /*A variable to store the released style*/
	lv_style_copy(&style_btn_rel, &lv_style_plain);         /*Initialize from a built-in style*/
	style_btn_rel.body.border.color = lv_color_hex3(0x269);
	style_btn_rel.body.border.width = 1;
	style_btn_rel.body.main_color = lv_color_hex3(0xADF);
	style_btn_rel.body.grad_color = lv_color_hex3(0x46B);
	style_btn_rel.body.shadow.width = 4;
	style_btn_rel.body.shadow.type = LV_SHADOW_BOTTOM;
	style_btn_rel.body.radius = LV_RADIUS_CIRCLE;
	style_btn_rel.text.color = lv_color_hex3(0xDEF);
	style_btn_rel.text.font  = &gb2312_puhui32;
	

	static lv_style_t style_btn_pr;                         /*A variable to store the pressed style*/
	lv_style_copy(&style_btn_pr, &style_btn_rel);           /*Initialize from the released style*/
	style_btn_pr.body.border.color = lv_color_hex3(0x46B);
	style_btn_pr.body.main_color = lv_color_hex3(0x8BD);
	style_btn_pr.body.grad_color = lv_color_hex3(0x24A);
	style_btn_pr.body.shadow.width = 2;
	style_btn_pr.text.color = lv_color_hex3(0xBCD);
	style_btn_pr.text.font  = &gb2312_puhui32;

	lv_obj_t * labelDialog = lv_label_create(scr, NULL);
	lv_obj_set_style(labelDialog, &tft_style_lable_rel);
    	

	if(uiCfg.dialogType == DIALOG_TYPE_FINISH_PRINT || uiCfg.dialogType == DIALOG_PAUSE_MESSAGE_RESUME)
	{
		lv_obj_t * btnOk = lv_btn_create(scr, NULL);     /*Add a button the current screen*/
		lv_obj_set_pos(btnOk, BTN_OK_X+90, BTN_OK_Y);                            /*Set its position*/
		lv_obj_set_size(btnOk, 100, 50);                          /*Set its size*/
		lv_obj_set_event_cb(btnOk, btn_ok_event_cb); 
		lv_btn_set_style(btnOk, LV_BTN_STYLE_REL, &style_btn_rel);    /*Set the button's released style*/
		lv_btn_set_style(btnOk, LV_BTN_STYLE_PR, &style_btn_pr);      /*Set the button's pressed style*/
		lv_obj_t * labelOk = lv_label_create(btnOk, NULL);          /*Add a label to the button*/
		lv_label_set_text(labelOk, print_file_dialog_menu.confirm);  /*Set the labels text*/

	}
	else if(uiCfg.dialogType == DIALOG_PAUSE_MESSAGE_WAITING
		||uiCfg.dialogType == DIALOG_PAUSE_MESSAGE_INSERT
		||uiCfg.dialogType == DIALOG_PAUSE_MESSAGE_HEAT)
	{
		lv_obj_t * btnOk = lv_btn_create(scr, NULL);     /*Add a button the current screen*/
		lv_obj_set_pos(btnOk, BTN_OK_X+90, BTN_OK_Y);                            /*Set its position*/
		lv_obj_set_size(btnOk, 100, 50);                          /*Set its size*/
		lv_obj_set_event_cb(btnOk, btn_ok_event_cb); 
		lv_btn_set_style(btnOk, LV_BTN_STYLE_REL, &style_btn_rel);    /*Set the button's released style*/
		lv_btn_set_style(btnOk, LV_BTN_STYLE_PR, &style_btn_pr);      /*Set the button's pressed style*/
		lv_obj_t * labelOk = lv_label_create(btnOk, NULL);          /*Add a label to the button*/
		lv_label_set_text(labelOk, print_file_dialog_menu.confirm);  /*Set the labels text*/		
	}
	else if(uiCfg.dialogType == DIALOG_PAUSE_MESSAGE_PAUSING
		||uiCfg.dialogType == DIALOG_PAUSE_MESSAGE_CHANGING
		||uiCfg.dialogType == DIALOG_PAUSE_MESSAGE_UNLOAD
		||uiCfg.dialogType == DIALOG_PAUSE_MESSAGE_LOAD
		||uiCfg.dialogType == DIALOG_PAUSE_MESSAGE_PURGE
		||uiCfg.dialogType == DIALOG_PAUSE_MESSAGE_RESUME
		||uiCfg.dialogType == DIALOG_PAUSE_MESSAGE_HEATING)
	{
		
	}
	else if(uiCfg.dialogType == WIFI_ENABLE_TIPS)
	{
		lv_obj_t * btnCancel = lv_btn_create(scr, NULL);     /*Add a button the current screen*/
		lv_obj_set_pos(btnCancel, BTN_OK_X+90, BTN_OK_Y);                            /*Set its position*/
		lv_obj_set_size(btnCancel, 100, 50);                          /*Set its size*/
		lv_obj_set_event_cb(btnCancel, btn_cancel_event_cb); 
		lv_btn_set_style(btnCancel, LV_BTN_STYLE_REL, &style_btn_rel);    /*Set the button's released style*/
		lv_btn_set_style(btnCancel, LV_BTN_STYLE_PR, &style_btn_pr);      /*Set the button's pressed style*/
		lv_obj_t * labelCancel = lv_label_create(btnCancel, NULL);          /*Add a label to the button*/
		lv_label_set_text(labelCancel, print_file_dialog_menu.cancle);
	}
	else if(uiCfg.dialogType == DIALOG_TRANSFER_NO_DEVICE)
	{
		lv_obj_t * btnCancel = lv_btn_create(scr, NULL);     /*Add a button the current screen*/
		lv_obj_set_pos(btnCancel, BTN_OK_X+90, BTN_OK_Y);                            /*Set its position*/
		lv_obj_set_size(btnCancel, 100, 50);                          /*Set its size*/
		lv_obj_set_event_cb(btnCancel, btn_cancel_event_cb); 
		lv_btn_set_style(btnCancel, LV_BTN_STYLE_REL, &style_btn_rel);    /*Set the button's released style*/
		lv_btn_set_style(btnCancel, LV_BTN_STYLE_PR, &style_btn_pr);      /*Set the button's pressed style*/
		lv_obj_t * labelCancel = lv_label_create(btnCancel, NULL);          /*Add a label to the button*/
		lv_label_set_text(labelCancel, print_file_dialog_menu.cancle);
	}
	else if(uiCfg.dialogType == DIALOG_TYPE_UPLOAD_FILE)
	{
		if(upload_result == 2)
		{
			lv_obj_t * btnCancel = lv_btn_create(scr, NULL);     /*Add a button the current screen*/
			lv_obj_set_pos(btnCancel, BTN_OK_X+90, BTN_OK_Y);                            /*Set its position*/
			lv_obj_set_size(btnCancel, 100, 50);                          /*Set its size*/
			lv_obj_set_event_cb(btnCancel, btn_cancel_event_cb); 
			lv_btn_set_style(btnCancel, LV_BTN_STYLE_REL, &style_btn_rel);    /*Set the button's released style*/
			lv_btn_set_style(btnCancel, LV_BTN_STYLE_PR, &style_btn_pr);      /*Set the button's pressed style*/
			lv_obj_t * labelCancel = lv_label_create(btnCancel, NULL);          /*Add a label to the button*/
			lv_label_set_text(labelCancel, print_file_dialog_menu.cancle);
		}
		else if(upload_result == 3)
		{
			lv_obj_t * btnOk = lv_btn_create(scr, NULL);     /*Add a button the current screen*/
			lv_obj_set_pos(btnOk, BTN_OK_X+90, BTN_OK_Y);                            /*Set its position*/
			lv_obj_set_size(btnOk, 100, 50);                          /*Set its size*/
			lv_obj_set_event_cb(btnOk, btn_ok_event_cb); 
			lv_btn_set_style(btnOk, LV_BTN_STYLE_REL, &style_btn_rel);    /*Set the button's released style*/
			lv_btn_set_style(btnOk, LV_BTN_STYLE_PR, &style_btn_pr);      /*Set the button's pressed style*/
			lv_obj_t * labelOk = lv_label_create(btnOk, NULL);          /*Add a label to the button*/
			lv_label_set_text(labelOk, print_file_dialog_menu.confirm);  /*Set the labels text*/						
		}
	}
	else
	{	
		lv_obj_t * btnOk = lv_btn_create(scr, NULL);     /*Add a button the current screen*/
		lv_obj_set_pos(btnOk, BTN_OK_X, BTN_OK_Y);                            /*Set its position*/
		lv_obj_set_size(btnOk, 100, 50);                          /*Set its size*/
		lv_obj_set_event_cb(btnOk, btn_ok_event_cb); 
		lv_btn_set_style(btnOk, LV_BTN_STYLE_REL, &style_btn_rel);    /*Set the button's released style*/
		lv_btn_set_style(btnOk, LV_BTN_STYLE_PR, &style_btn_pr);      /*Set the button's pressed style*/
		lv_obj_t * labelOk = lv_label_create(btnOk, NULL);          /*Add a label to the button*/
		
		lv_obj_t * btnCancel = lv_btn_create(scr, NULL);     /*Add a button the current screen*/
		lv_obj_set_pos(btnCancel, BTN_CANCEL_X, BTN_CANCEL_Y);                            /*Set its position*/
		lv_obj_set_size(btnCancel, 100, 50);                          /*Set its size*/
		lv_obj_set_event_cb(btnCancel, btn_cancel_event_cb); 
		lv_btn_set_style(btnCancel, LV_BTN_STYLE_REL, &style_btn_rel);    /*Set the button's released style*/
		lv_btn_set_style(btnCancel, LV_BTN_STYLE_PR, &style_btn_pr);      /*Set the button's pressed style*/
		lv_obj_t * labelCancel = lv_label_create(btnCancel, NULL);          /*Add a label to the button*/

		if(uiCfg.dialogType == DIALOG_PAUSE_MESSAGE_OPTION)
		{
			lv_label_set_text(labelOk, pause_msg_menu.purgeMore);  /*Set the labels text*/
			lv_label_set_text(labelCancel, pause_msg_menu.continuePrint);
		}
		else
		{
			lv_label_set_text(labelOk, print_file_dialog_menu.confirm);  /*Set the labels text*/
			lv_label_set_text(labelCancel, print_file_dialog_menu.cancle);
		}
	}
	if(uiCfg.dialogType == DIALOG_TYPE_PRINT_FILE)
	{
		lv_label_set_text(labelDialog, print_file_dialog_menu.print_file);
		lv_obj_align(labelDialog, NULL, LV_ALIGN_CENTER, 0, -20);
		
		lv_obj_t * labelFile = lv_label_create(scr, NULL);
		lv_obj_set_style(labelFile, &tft_style_lable_rel);
	    	
		lv_label_set_text(labelFile, list_file.long_name[sel_id]);
		lv_obj_align(labelFile, NULL, LV_ALIGN_CENTER, 0, -60);
	}    
	else if(uiCfg.dialogType == DIALOG_TYPE_STOP)
	{
		lv_label_set_text(labelDialog, print_file_dialog_menu.cancle_print);
		lv_obj_align(labelDialog, NULL, LV_ALIGN_CENTER, 0, -20);
	}
	else if(uiCfg.dialogType == DIALOG_TYPE_FINISH_PRINT)
	{
		lv_label_set_text(labelDialog, print_file_dialog_menu.print_finish);
		lv_obj_align(labelDialog, NULL, LV_ALIGN_CENTER, 0, -20);
	}
	else if(uiCfg.dialogType == DIALOG_PAUSE_MESSAGE_PAUSING)
	{
		lv_label_set_text(labelDialog, pause_msg_menu.pausing);
		lv_obj_align(labelDialog, NULL, LV_ALIGN_CENTER, 0, -20);
	}
	else if(uiCfg.dialogType == DIALOG_PAUSE_MESSAGE_CHANGING)
	{
		lv_label_set_text(labelDialog, pause_msg_menu.changing);
		lv_obj_align(labelDialog, NULL, LV_ALIGN_CENTER, 0, -20);
	}
	else if(uiCfg.dialogType == DIALOG_PAUSE_MESSAGE_UNLOAD)
	{
		lv_label_set_text(labelDialog, pause_msg_menu.unload);
		lv_obj_align(labelDialog, NULL, LV_ALIGN_CENTER, 0, -20);
	}
	else if(uiCfg.dialogType == DIALOG_PAUSE_MESSAGE_WAITING)
	{
		lv_label_set_text(labelDialog, pause_msg_menu.waiting);
		lv_obj_align(labelDialog, NULL, LV_ALIGN_CENTER, 0, -20);
	}
	else if(uiCfg.dialogType == DIALOG_PAUSE_MESSAGE_INSERT)
	{
		lv_label_set_text(labelDialog, pause_msg_menu.insert);
		lv_obj_align(labelDialog, NULL, LV_ALIGN_CENTER, 0, -20);
	}
	else if(uiCfg.dialogType == DIALOG_PAUSE_MESSAGE_LOAD)
	{
		lv_label_set_text(labelDialog, pause_msg_menu.load);
		lv_obj_align(labelDialog, NULL, LV_ALIGN_CENTER, 0, -20);
	}
	else if(uiCfg.dialogType == DIALOG_PAUSE_MESSAGE_PURGE)
	{
		lv_label_set_text(labelDialog, pause_msg_menu.purge);
		lv_obj_align(labelDialog, NULL, LV_ALIGN_CENTER, 0, -20);
	}
	else if(uiCfg.dialogType == DIALOG_PAUSE_MESSAGE_RESUME)
	{
		lv_label_set_text(labelDialog, pause_msg_menu.resume);
		lv_obj_align(labelDialog, NULL, LV_ALIGN_CENTER, 0, -20);
	}
	else if(uiCfg.dialogType == DIALOG_PAUSE_MESSAGE_HEAT)
	{
		lv_label_set_text(labelDialog, pause_msg_menu.heat);
		lv_obj_align(labelDialog, NULL, LV_ALIGN_CENTER, 0, -20);
	}
	else if(uiCfg.dialogType == DIALOG_PAUSE_MESSAGE_HEATING)
	{
		lv_label_set_text(labelDialog, pause_msg_menu.heating);
		lv_obj_align(labelDialog, NULL, LV_ALIGN_CENTER, 0, -20);
	}
	else if(uiCfg.dialogType == DIALOG_PAUSE_MESSAGE_OPTION)
	{
		lv_label_set_text(labelDialog, pause_msg_menu.option);
		lv_obj_align(labelDialog, NULL, LV_ALIGN_CENTER, 0, -20);
	}
	else if(uiCfg.dialogType == DIALOG_STORE_EEPROM_TIPS)
	{
		lv_label_set_text(labelDialog, eeprom_menu.storeTips);
		lv_obj_align(labelDialog, NULL, LV_ALIGN_CENTER, 0, -20);
	}
	else if(uiCfg.dialogType == DIALOG_READ_EEPROM_TIPS)
	{
		lv_label_set_text(labelDialog, eeprom_menu.readTips);
		lv_obj_align(labelDialog, NULL, LV_ALIGN_CENTER, 0, -20);
	}
	else if(uiCfg.dialogType == DIALOG_REVERT_EEPROM_TIPS)
	{
		lv_label_set_text(labelDialog, eeprom_menu.revertTips);
		lv_obj_align(labelDialog, NULL, LV_ALIGN_CENTER, 0, -20);
	}
	else if(uiCfg.dialogType == DIALOG_WIFI_CONFIG_TIPS)
	{
		lv_label_set_text(labelDialog, machine_menu.wifiConfigTips);
		lv_obj_align(labelDialog, NULL, LV_ALIGN_CENTER, 0, -20);
	}
	else if(uiCfg.dialogType == WIFI_ENABLE_TIPS)
	{
		lv_label_set_text(labelDialog, print_file_dialog_menu.wifi_enable_tips);
		lv_obj_align(labelDialog, NULL, LV_ALIGN_CENTER, 0, -20);
	}
	else if(uiCfg.dialogType == DIALOG_TRANSFER_NO_DEVICE)
	{
		lv_label_set_text(labelDialog, DIALOG_UPDATE_NO_DEVICE_EN);
		lv_obj_align(labelDialog, NULL, LV_ALIGN_CENTER, 0, -20);
	}
	else if(uiCfg.dialogType == DIALOG_TYPE_UPLOAD_FILE)
	{
		if(upload_result == 1)
		{
			lv_label_set_text(labelDialog, DIALOG_UPLOAD_ING_EN);
			lv_obj_align(labelDialog, NULL, LV_ALIGN_CENTER, 0, -20);
		}
		else if(upload_result == 2)
		{
			lv_label_set_text(labelDialog, DIALOG_UPLOAD_ERROR_EN);
			lv_obj_align(labelDialog, NULL, LV_ALIGN_CENTER, 0, -20);
		}
		else if(upload_result == 3)
		{
			char buf[200];
			int _index = 0;
			
			memset(buf,0,sizeof(200));
			
			strcpy(buf, DIALOG_UPLOAD_FINISH_EN);
			_index = strlen(buf);
			buf[_index] = '\n';
			_index++;
			strcat(buf, DIALOG_UPLOAD_SIZE_EN);
			
			_index = strlen(buf);
			buf[_index] = ':';
			_index++;
			sprintf(&buf[_index], " %.1d KBytes\n", upload_size / 1024);

			strcat(buf, DIALOG_UPLOAD_TIME_EN);
			_index = strlen(buf);
			buf[_index] = ':';
			_index++;
			sprintf(&buf[_index], " %d s\n", upload_time);
			
			strcat(buf, DIALOG_UPLOAD_SPEED_EN);
			_index = strlen(buf);
			buf[_index] = ':';
			_index++;
			sprintf(&buf[_index], " %d KBytes/s\n", upload_size / upload_time / 1024);	

			lv_label_set_text(labelDialog, buf);
			lv_obj_align(labelDialog, NULL, LV_ALIGN_CENTER, 0, -20);
			
		}
	}
	
}

void lv_clear_dialog()
{
	lv_obj_del(scr);
}

#endif
