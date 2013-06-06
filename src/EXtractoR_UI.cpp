//
//	EXtractoR
//		by Brendan Bolles <brendan@fnordware.com>
//
//	extract float channels
//
//	Part of the fnord OpenEXR tools
//		http://www.fnordware.com/OpenEXR/
//
//
//	mmm...our custom UI
//
//

#include "EXtractoR.h"

#include "OpenEXR_UTF.h"


static PF_Err
DrawEvent(	
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output,
	PF_EventExtra	*event_extra)
{
	PF_Err			err		=	PF_Err_NONE;
	
	AEGP_SuiteHandler suites(in_data->pica_basicP);
	

	event_extra->evt_out_flags = 0;
	

	if (!(event_extra->evt_in_flags & PF_EI_DONT_DRAW) && params[EXTRACT_DATA]->u.arb_d.value != NULL)
	{
		if(PF_EA_CONTROL == event_extra->effect_win.area)
		{
			ArbitraryData *arb_data = (ArbitraryData *)PF_LOCK_HANDLE(params[EXTRACT_DATA]->u.arb_d.value);
			ExtractStatus *extract_status = (ExtractStatus *)PF_LOCK_HANDLE(in_data->sequence_data);
		
		#define MAX_MESSAGE_LEN 127
			A_char	red_message[MAX_MESSAGE_LEN],
					green_message[MAX_MESSAGE_LEN],
					blue_message[MAX_MESSAGE_LEN],
					alpha_message[MAX_MESSAGE_LEN];
					
			A_char	*channel_message[4] = {	red_message,
											green_message,
											blue_message,
											alpha_message };
			
			ChannelStatus *channel_status[4] = {	&extract_status->red,
													&extract_status->green,
													&extract_status->blue,
													&extract_status->alpha };
												
			ChannelData *channel_data[4] = {	&arb_data->red,
												&arb_data->green,
												&arb_data->blue,
												&arb_data->alpha };
		
			// set up the messages
			for(int i=0; i<4; i++)
			{
				if(channel_data[i]->action == DO_COPY)
				{
					strcpy(channel_message[i], "(copy)");
				}
				else if(channel_data[i]->action == DO_EXTRACT)
				{
					// start with the name
					strcpy(channel_message[i], channel_data[i]->name);
					
					// process status returned from Render()
					if(channel_status[i]->status == STATUS_NOT_FOUND)
					{
						strcat(channel_message[i], " (not found)");
					}
					else if(channel_status[i]->status == STATUS_NO_CHANNELS)
					{
						strcat(channel_message[i], " (none available)");
					}
					else if(channel_status[i]->status == STATUS_ERROR)
					{
						strcat(channel_message[i], " (error)");
					}
					// can't change params during draw, need a click
					//else if(channel_status[i]->status == STATUS_INDEX_CHANGE)
					//{
					//	channel_data[i]->index = channel_status[i]->index;
					//	params[EXTRACT_DATA]->uu.change_flags = PF_ChangeFlag_CHANGED_VALUE;
					//}
				}
			}
			
			PF_Rect control_rect = event_extra->effect_win.current_frame;
			
			
#if PF_AE_PLUG_IN_VERSION < PF_AE100_PLUG_IN_VERSION

		// old-school Carbon/Win32 drawing code

		#ifdef WIN_ENV
			void*			dp = (*(event_extra->contextH))->cgrafptr;
			HDC				ecw_hdc	=	0;
			PF_GET_CGRAF_DATA(dp, PF_CGrafData_HDC, (void **)&ecw_hdc);
			PF_Point		pen;
		#endif

			PF_App_Color	pf_bg_color;
			suites.AppSuite()->PF_AppGetBgColor(&pf_bg_color);
			
		#ifdef MAC_ENV
			RGBColor titleColor, itemColor;
			RGBColor		bg_color;
	
			bg_color.red	=	pf_bg_color.red;
			bg_color.green	=	pf_bg_color.green;
			bg_color.blue	=	pf_bg_color.blue;
			
			RGBBackColor(&bg_color);
		
			EraseRect(&control_rect);
			
			GetForeColor(&itemColor);
			
			titleColor.red   = TITLE_COMP(itemColor.red);
			titleColor.green = TITLE_COMP(itemColor.green);
			titleColor.blue  = TITLE_COMP(itemColor.blue);
				
			RGBForeColor(&titleColor);
		#endif

		#ifdef MAC_ENV
		#define DRAW_STRING(STRING)	DrawText(STRING, 0, strlen(STRING));
		#define MOVE_TO(H, V)		MoveTo( (H), (V) );
		#else
		#define DRAW_STRING(STRING) \
			do{ \
				WCHAR u_string[100]; \
				UTF8toUTF16(STRING, (utf16_char *)u_string, 99); \
				TextOut(ecw_hdc, pen.h, pen.v, u_string, strlen(STRING)); \
			}while(0);
		#define MOVE_TO(H, V)		MoveToEx(ecw_hdc, (H), (V), NULL); pen.h = (H); pen.v = (V);
		#endif

		
#else // PF_AE_PLUG_IN_VERSION >= PF_AE100_PLUG_IN_VERSION

			// new-school Drawbot drawing code
			
			DRAWBOT_DrawRef			drawbot_ref = NULL;
			suites.PFEffectCustomUISuite()->PF_GetDrawingReference(event_extra->contextH, &drawbot_ref);
			
			DRAWBOT_SupplierRef		supplier_ref = NULL;
			DRAWBOT_SurfaceRef		surface_ref = NULL;

			suites.DBDrawbotSuite()->GetSupplier(drawbot_ref, &supplier_ref);
			suites.DBDrawbotSuite()->GetSurface(drawbot_ref, &surface_ref);
				
			DRAWBOT_ColorRGBA title_color = {0.8f, 0.8f, 0.8f, 1.0f};
			DRAWBOT_ColorRGBA item_color = title_color;
			
			item_color.red *= (1.0 - kUI_TITLE_COLOR_SCALEDOWN);
			item_color.green *= (1.0 - kUI_TITLE_COLOR_SCALEDOWN);
			item_color.blue *= (1.0 - kUI_TITLE_COLOR_SCALEDOWN);
			
			DRAWBOT_BrushP title_brushP(suites.DBSupplierSuite(), supplier_ref, &title_color);
			DRAWBOT_BrushP item_brushP(suites.DBSupplierSuite(), supplier_ref, &item_color);
			
			float default_font_sizeF = 0.0;
			suites.DBSupplierSuite()->GetDefaultFontSize(supplier_ref, &default_font_sizeF);
			
			DRAWBOT_FontP fontP(suites.DBSupplierSuite(), supplier_ref, default_font_sizeF);
			
			DRAWBOT_PointF32 text_origin = {0.f, 0.f};
			
			DRAWBOT_BrushP *current_brush = &title_brushP;
			
		#define MOVE_TO(H, V) \
			text_origin.x = (H) + 0.5f; \
			text_origin.y = (V) + 0.5f;
		
		#define DRAW_STRING(STRING) \
			do{ \
				DRAWBOT_UTF16Char u_str[100]; \
				UTF8toUTF16(STRING, u_str, 99); \
				suites.DBSurfaceSuite()->DrawString(surface_ref, current_brush->Get(), fontP.Get(), u_str, \
						&text_origin, kDRAWBOT_TextAlignment_Default, kDRAWBOT_TextTruncation_None, 0.0f); \
			}while(0);
			
#endif //PF_AE_PLUG_IN_VERSION >= PF_AE100_PLUG_IN_VERSION


			// Property Headings
			MOVE_TO(RIGHT_STATUS(control_rect.left), DOWN_PLACE(INFO_RED, control_rect.top) );
			DRAW_STRING("Red");
			
			MOVE_TO(RIGHT_STATUS(control_rect.left), DOWN_PLACE(INFO_GREEN, control_rect.top) );
			DRAW_STRING("Green");

			MOVE_TO(RIGHT_STATUS(control_rect.left), DOWN_PLACE(INFO_BLUE, control_rect.top) );
			DRAW_STRING("Blue");

			MOVE_TO(RIGHT_STATUS(control_rect.left), DOWN_PLACE(INFO_ALPHA, control_rect.top) );
			DRAW_STRING("Alpha");
			
			//Property Items
			PF_Point prop_origin;
			prop_origin.v = control_rect.top;
			
	#if PF_AE_PLUG_IN_VERSION < PF_AE100_PLUG_IN_VERSION
		#ifdef MAC_ENV
			RGBForeColor(&itemColor);
			prop_origin.h = control_rect.left + TextWidth("Alpha", 0, strlen("Alpha"));
		#else
			SIZE text_dim = {15, 8};
			GetTextExtentPoint(ecw_hdc, TEXT("Alpha"), strlen("Alpha"), &text_dim);
			prop_origin.h = control_rect.left + text_dim.cx;
		#endif
	#else
			current_brush = &item_brushP;
			prop_origin.h = control_rect.left + 50;
	#endif
			
			MOVE_TO(RIGHT_PROP(prop_origin.h), DOWN_PLACE(INFO_RED, prop_origin.v) );
			DRAW_STRING(red_message);
			
			MOVE_TO(RIGHT_PROP(prop_origin.h), DOWN_PLACE(INFO_GREEN, prop_origin.v) );
			DRAW_STRING(green_message);

			MOVE_TO(RIGHT_PROP(prop_origin.h), DOWN_PLACE(INFO_BLUE, prop_origin.v) );
			DRAW_STRING(blue_message);

			MOVE_TO(RIGHT_PROP(prop_origin.h), DOWN_PLACE(INFO_ALPHA, prop_origin.v) );
			DRAW_STRING(alpha_message);
			

			event_extra->evt_out_flags = PF_EO_HANDLED_EVENT;

			PF_UNLOCK_HANDLE(params[EXTRACT_DATA]->u.arb_d.value);
			PF_UNLOCK_HANDLE(in_data->sequence_data);
		}
	}

	return err;
}


PF_Err
HandleEvent ( 
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output,
	PF_EventExtra	*extra )
{
	PF_Err		err		= PF_Err_NONE;
	
	if (!err) 
	{
		switch (extra->e_type) 
		{
			case PF_Event_DRAW:
				err =	DrawEvent(in_data, out_data, params, output, extra);
				break;
			
			case PF_Event_DO_CLICK:
				err = DoDialog(in_data, out_data, params, output);
				extra->evt_out_flags = PF_EO_HANDLED_EVENT;
				break;
				
			case PF_Event_ADJUST_CURSOR:
			#if defined(MAC_ENV)
				#if PF_AE_PLUG_IN_VERSION >= PF_AE100_PLUG_IN_VERSION
				SetMickeyCursor(); // the cute mickey mouse hand
				#else
				SetThemeCursor(kThemePointingHandCursor);
				#endif
				extra->u.adjust_cursor.set_cursor = PF_Cursor_CUSTOM;
			#else
				extra->u.adjust_cursor.set_cursor = PF_Cursor_FINGER_POINTER;
			#endif
				extra->evt_out_flags = PF_EO_HANDLED_EVENT;
				break;
		}
	}
	
	return err;
}
