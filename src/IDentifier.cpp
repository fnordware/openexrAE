//
//	IDentifier
//		by Brendan Bolles <brendan@fnordware.com>
//
//	extract discrete UINT channels
//
//	Part of the fnord OpenEXR tools
//		http://www.fnordware.com/OpenEXR/
//
//

#include "IDentifier.h"

#include <limits.h>


static PF_Err 
About (	
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output )
{
	PF_SPRINTF(	out_data->return_msg, 
				"%s - %s\r\rwritten by %s\r\rv%d.%d - %s\r\r%s\r%s",
				NAME,
				DESCRIPTION,
				AUTHOR, 
				MAJOR_VERSION, 
				MINOR_VERSION,
				RELEASE_DATE,
				COPYRIGHT,
				WEBSITE);
				
	return PF_Err_NONE;
}

static PF_Err 
GlobalSetup (	
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output )
{
	//	We do very little here.
		
	out_data->my_version 	= 	PF_VERSION(	MAJOR_VERSION, 
											MINOR_VERSION,
											BUG_VERSION, 
											STAGE_VERSION, 
											BUILD_VERSION);

	out_data->out_flags 	= 	PF_OutFlag_DEEP_COLOR_AWARE		|
								PF_OutFlag_PIX_INDEPENDENT		|
							#ifdef WIN_ENV
								PF_OutFlag_KEEP_RESOURCE_OPEN	|
							#endif
								PF_OutFlag_CUSTOM_UI			|
								PF_OutFlag_USE_OUTPUT_EXTENT;

	out_data->out_flags2 	=	PF_OutFlag2_SUPPORTS_SMART_RENDER	|
								PF_OutFlag2_FLOAT_COLOR_AWARE;
	
	return PF_Err_NONE;
}

static PF_Err
ParamsSetup(
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output)
{
	PF_Err 			err = PF_Err_NONE;
	PF_ParamDef		def;


	// readout
	AEFX_CLR_STRUCT(def);
	 // we can time_vary once we're willing to print and scan ArbData text
	def.flags = PF_ParamFlag_CANNOT_TIME_VARY;
	
	ArbNewDefault(in_data, out_data, ARB_REFCON, &def.u.arb_d.dephault);
	
	PF_ADD_ARBITRARY("Channel Info (Click for Dialog)",
						kUI_CONTROL_WIDTH,
						kUI_CONTROL_HEIGHT,
						PF_PUI_CONTROL,
						def.u.arb_d.dephault,
						ARBITRARY_DATA_ID,
						ARB_REFCON);

	AEFX_CLR_STRUCT(def);
	PF_ADD_POPUP(	"Display",
					DISPLAY_NUM_OPTIONS, //number of choices
					DISPLAY_COLORS, //default
					DISPLAY_MENU_STR,
					DISPLAY_ID);

	AEFX_CLR_STRUCT(def);
	PF_ADD_SLIDER(	"ID",
					0, INT_MAX,
					0, 32,
					0, //default
					CHANNEL_ID_ID);


	out_data->num_params = EXTRACT_NUM_PARAMS;


	// register custom UI
	if (!err) 
	{
		PF_CustomUIInfo			ci;

		AEFX_CLR_STRUCT(ci);
		
		ci.events				= PF_CustomEFlag_EFFECT;
 		
		ci.comp_ui_width		= ci.comp_ui_height = 0;
		ci.comp_ui_alignment	= PF_UIAlignment_NONE;
		
		ci.layer_ui_width		= 0;
		ci.layer_ui_height		= 0;
		ci.layer_ui_alignment	= PF_UIAlignment_NONE;
		
		ci.preview_ui_width		= 0;
		ci.preview_ui_height	= 0;
		ci.layer_ui_alignment	= PF_UIAlignment_NONE;

		err = (*(in_data->inter.register_ui))(in_data->effect_ref, &ci);
	}


	return err;
}


static PF_Err
SequenceSetup (
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output )
{
	PF_Err err = PF_Err_NONE;
	
	ChannelStatus *channel_status = NULL;
	
	// set up sequence data (our LUT)
	if( (in_data->sequence_data == NULL) )
	{
		out_data->sequence_data = PF_NEW_HANDLE( sizeof(ChannelStatus) );
		
		channel_status = (ChannelStatus *)PF_LOCK_HANDLE(out_data->sequence_data);

		// set defaults
		channel_status->status = STATUS_UNKNOWN;
		
		PF_UNLOCK_HANDLE(out_data->sequence_data);
	}
	else // reset pre-existing sequence data
	{
		if( PF_GET_HANDLE_SIZE(in_data->sequence_data) != sizeof(ChannelStatus) )
		{
			PF_RESIZE_HANDLE(sizeof(ChannelStatus), &in_data->sequence_data);
		}
			
		channel_status = (ChannelStatus *)PF_LOCK_HANDLE(in_data->sequence_data);
		
		// set defaults
		channel_status->status = STATUS_UNKNOWN;
		
		PF_UNLOCK_HANDLE(in_data->sequence_data);
	}
	
	return err;
}


static PF_Err 
SequenceSetdown (
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output )
{
	PF_Err err = PF_Err_NONE;
	
	if(in_data->sequence_data)
		PF_DISPOSE_HANDLE(in_data->sequence_data);

	return err;
}


static PF_Err 
SequenceFlatten (
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output )
{
	PF_Err err = PF_Err_NONE;
/*
	if(in_data->sequence_data)
	{
		ChannelInfoPtr channel_info = (ChannelInfoPtr)PF_LOCK_HANDLE(in_data->sequence_data);

		PF_UNLOCK_HANDLE(in_data->sequence_data);
	}
*/
	return err;
}


static
PF_Boolean IsEmptyRect(const PF_LRect *r){
	return (r->left >= r->right) || (r->top >= r->bottom);
}

#ifndef mmin
	#define mmin(a,b) ((a) < (b) ? (a) : (b))
	#define mmax(a,b) ((a) > (b) ? (a) : (b))
#endif

static
void UnionLRect(const PF_LRect *src, PF_LRect *dst)
{
	if (IsEmptyRect(dst)) {
		*dst = *src;
	} else if (!IsEmptyRect(src)) {
		dst->left 	= mmin(dst->left, src->left);
		dst->top  	= mmin(dst->top, src->top);
		dst->right 	= mmax(dst->right, src->right);
		dst->bottom = mmax(dst->bottom, src->bottom);
	}
}

static PF_Err
PreRender(
	PF_InData				*in_data,
	PF_OutData				*out_data,
	PF_PreRenderExtra		*extra)
{
	PF_Err err = PF_Err_NONE;
	PF_RenderRequest req = extra->input->output_request;
	PF_CheckoutResult in_result;
	
	req.preserve_rgb_of_zero_alpha = TRUE;	//	Hey, we care.

	ERR(extra->cb->checkout_layer(	in_data->effect_ref,
									EXTRACT_INPUT,
									EXTRACT_INPUT,
									&req,
									in_data->current_time,
									in_data->time_step,
									in_data->time_scale,
									&in_result));


	UnionLRect(&in_result.result_rect, 		&extra->output->result_rect);
	UnionLRect(&in_result.max_result_rect, 	&extra->output->max_result_rect);	
	
	//	Notice something missing, namely the PF_CHECKIN_PARAM to balance
	//	the old-fashioned PF_CHECKOUT_PARAM, above? 
	
	//	For SmartFX, AE automagically checks in any params checked out 
	//	during PF_Cmd_SMART_PRE_RENDER, new or old-fashioned.
	
	return err;
}

#pragma mark-

static void SetColors(PF_Pixel &pixel, unsigned int id)
{
#define NUM_COLORS 18

	// colors we'll cycle through for the ID numbers
	static const PF_Pixel colors8[NUM_COLORS] =
	{	{255,	255,	0,		0	},
		{255,	0,		255,	0	},
		{255,	0,		0,		255	},
		
		{255,	255,	128,	0	},
		{255,	0,		255,	128	},
		{255,	128,	0,		255	},

		{255,	128,	128,	0	},
		{255,	0,		128,	128	},
		{255,	128,	0,		128	},

		{255,	255,	0,		255	},
		{255,	255,	255,	0	},
		{255,	0,		255,	255	},

		{255,	255,	0,		128	},
		{255,	128,	255,	0	},
		{255,	0,		128,	255	},
		
		{255,	128,	0,		128	},
		{255,	128,	128,	0	},
		{255,	0,		128,	128	}
	};
	
	pixel = colors8[id % NUM_COLORS];
}

static void SetColors(PF_Pixel16 &pixel, unsigned int id)
{
	PF_Pixel pixel8;
	
	SetColors(pixel8, id);
	
	pixel.alpha	= PF_MAX_CHAN16;
	pixel.red	= CONVERT8TO16(pixel8.red);
	pixel.green	= CONVERT8TO16(pixel8.green);
	pixel.blue	= CONVERT8TO16(pixel8.blue);
}

static void SetColors(PF_PixelFloat &pixel, unsigned int id)
{
	PF_Pixel pixel8;
	
	SetColors(pixel8, id);
	
	pixel.alpha	= 1.0f;
	pixel.red	= (float)pixel8.red / 255.f;
	pixel.green	= (float)pixel8.green / 255.f;
	pixel.blue	= (float)pixel8.blue / 255.f;
}


template <typename T>
static inline T
OnOff(bool on);

template <>
static inline A_u_char
OnOff<A_u_char>(bool on)
{
	return (on ? PF_MAX_CHAN8 : 0);
}

template <>
static inline A_u_short
OnOff<A_u_short>(bool on)
{
	return (on ? PF_MAX_CHAN16 : 0);
}

template <>
static inline PF_FpShort
OnOff<PF_FpShort>(bool on)
{
	return (on ? 1.f : 0.f);
}


template <typename T>
static inline T
Raw(unsigned int id);

template <>
static inline A_u_char
Raw<A_u_char>(unsigned int id)
{
	return (id > PF_MAX_CHAN8 ? PF_MAX_CHAN8 : id);
}

template <>
static inline A_u_short
Raw<A_u_short>(unsigned int id)
{
	if(id > PF_MAX_CHAN8)
		id = PF_MAX_CHAN8;
	
	return CONVERT8TO16(id);
}

template <>
static inline PF_FpShort
Raw<PF_FpShort>(unsigned int id)
{
	return (float)id / 255.f;
}


typedef struct {
	PF_InData *in_data;
	void *in_buffer;
	size_t in_rowbytes;
	void *out_buffer;
	size_t out_rowbytes;
	int display_type;
	int channel_id;
	int width;
} IterateData;

template <typename IntType, typename PixType, typename ChanType>
static PF_Err
CopyIDChannel_Iterate(void *refconPV,
						A_long thread_indexL,
						A_long i,
						A_long iterationsL)
{
	PF_Err err = PF_Err_NONE;
	
	IterateData *i_data = (IterateData *)refconPV;
	PF_InData *in_data = i_data->in_data;
	
	IntType *in_pix = (IntType *)((char *)i_data->in_buffer + (i * i_data->in_rowbytes)); 
	PixType *out_pix = (PixType *)((char *)i_data->out_buffer + (i * i_data->out_rowbytes));
	
	for(int x=0; x < i_data->width; x++)
	{
		if(i_data->display_type == DISPLAY_COLORS)
		{
			SetColors(*out_pix, *in_pix);
		}
		else if(i_data->display_type == DISPLAY_LUMA_MATTE)
		{
			out_pix->alpha	= OnOff<ChanType>(true);
			out_pix->red = out_pix->green = out_pix->blue = OnOff<ChanType>(*in_pix == i_data->channel_id);
		}
		else if(i_data->display_type == DISPLAY_ALPHA_MATTE)
		{
			out_pix->alpha = OnOff<ChanType>(*in_pix == i_data->channel_id);
		}
		else if(i_data->display_type == DISPLAY_RAW)
		{
			out_pix->alpha	= OnOff<ChanType>(true);
			out_pix->red = out_pix->green = out_pix->blue = Raw<ChanType>(*in_pix);
		}
		
		in_pix++;
		out_pix++;
	}
	
#ifdef NDEBUG
	if(thread_indexL == 0)
		err = PF_ABORT(in_data);
#endif

	return err;
}


template <typename IntType>
static PF_Err
CopyIDChannel(PF_InData *in_data,
				IntType *input, size_t in_rowbytes,
				PF_PixelPtr	output, PF_PixelFormat out_format, size_t out_rowbytes,
				int display_type, unsigned int channel_id,
				int width, int height)
{
	PF_Err err = PF_Err_NONE;

	AEGP_SuiteHandler suites(in_data->pica_basicP);
	

	IterateData i_data = { in_data, (void *)input, in_rowbytes, (void *)output, out_rowbytes, display_type, channel_id, width };
	
	if(out_format == PF_PixelFormat_ARGB128)
	{
		err = suites.PFIterate8Suite()->iterate_generic(height, &i_data, CopyIDChannel_Iterate<IntType, PF_PixelFloat, PF_FpShort>);
	}
	else if(out_format == PF_PixelFormat_ARGB64)
	{
		err = suites.PFIterate8Suite()->iterate_generic(height, &i_data, CopyIDChannel_Iterate<IntType, PF_Pixel16, A_u_short>);
	}
	else if(out_format == PF_PixelFormat_ARGB32)
	{
		err = suites.PFIterate8Suite()->iterate_generic(height, &i_data, CopyIDChannel_Iterate<IntType, PF_Pixel, A_u_char>);
	}
	
	return err;
}


#pragma mark-


static PF_Err
DoRender(
		PF_InData		*in_data,
		PF_EffectWorld 	*input,
		PF_ParamDef		*EXTRACT_data, 
		PF_ParamDef		*EXTRACT_display,
		PF_ParamDef		*EXTRACT_id,
		PF_OutData		*out_data,
		PF_EffectWorld	*output)
{
	PF_Err				err 	= PF_Err_NONE,
						err2 	= PF_Err_NONE,
						err3	= PF_Err_NONE;
	PF_Point			origin;
	PF_Rect				src_rect, areaR;
	PF_PixelFormat		format	=	PF_PixelFormat_INVALID;
	
	AEGP_SuiteHandler suites(in_data->pica_basicP);
 
	PF_WorldSuite2		*wsP	=	suites.PFWorldSuite();
	PF_ChannelSuite1	*cs		=	suites.PFChannelSuite();

	src_rect.left 	= -in_data->output_origin_x;
	src_rect.top 	= -in_data->output_origin_y;
	src_rect.bottom = src_rect.top + output->height;
	src_rect.right 	= src_rect.left + output->width;

	if (!err){
		//origin.h = in_data->output_origin_x;
		//origin.v = in_data->output_origin_y;
		origin.h = output->origin_x;
		origin.v = output->origin_y;

		areaR.top		= 
		areaR.left 		= 0;
		areaR.right		= 1;
		areaR.bottom	= output->height;



		ERR(wsP->PF_GetPixelFormat(output, &format));

		// some sizes we'll need
		size_t sizeof_pixel, sizeof_channel;

		if(format == PF_PixelFormat_ARGB32)
		{
			sizeof_pixel = sizeof(PF_Pixel);
			sizeof_channel = sizeof(A_u_char);
		}
		else if(format == PF_PixelFormat_ARGB64)
		{
			sizeof_pixel = sizeof(PF_Pixel16);
			sizeof_channel = sizeof(A_u_short);
		}
		if(format == PF_PixelFormat_ARGB128)
		{
			sizeof_pixel = sizeof(PF_Pixel32);
			sizeof_channel = sizeof(PF_FpShort);
		}

		// start with a copy
		//PF_COPY(input, output, &src_rect, NULL);
	
		ArbitraryData *arb_data = (ArbitraryData *)PF_LOCK_HANDLE(EXTRACT_data->u.arb_d.value);
		ChannelStatus *channel_status = (ChannelStatus *)PF_LOCK_HANDLE(in_data->sequence_data);


		PF_Boolean	found = FALSE;
		
		PF_ChannelRef chan_ref;
		PF_ChannelDesc chan_desc;
		
		A_long num_channels;
		cs->PF_GetLayerChannelCount(in_data->effect_ref, EXTRACT_INPUT, &num_channels);
		
		if(num_channels > 0) // don't bother if there's no channels
		{
			// let's assume normal for now
			channel_status->status = STATUS_NORMAL;
			
			// get the channel
			cs->PF_GetLayerChannelIndexedRefAndDesc(in_data->effect_ref,
													EXTRACT_INPUT,
													MAX(0, MIN(arb_data->index, num_channels-1)),
													&found,
													&chan_ref,
													&chan_desc);
			
			
			// found isn't really what it used to be
			found = (found && chan_desc.channel_type && chan_desc.data_type);
			
			// does the channel name match what we stored?
			if(found && 0 != strcmp(arb_data->name, chan_desc.name) )
			{
				// uh oh, let's go searching for one that does
				int j;
				
				PF_Boolean found_replacement = FALSE;
				
				for(j=0; j<num_channels && !found_replacement; j++)
				{
					// get the channel
					cs->PF_GetLayerChannelIndexedRefAndDesc(in_data->effect_ref,
															EXTRACT_INPUT,
															j,
															&found,
															&chan_ref,
															&chan_desc);
															
					// found isn't really what it used to be
					found = (found && chan_desc.channel_type && chan_desc.data_type);
					
					if(found && !strcmp(arb_data->name, chan_desc.name) )
					{
						arb_data->index = j; // this won't actually get set in the parameters
						found_replacement = TRUE;
						//EXTRACT_data->uu.change_flags = PF_ChangeFlag_CHANGED_VALUE;
						
						// report back on the new ID
						channel_status->status = STATUS_INDEX_CHANGE;
						channel_status->index = arb_data->index;
					}
				}

				if(!found_replacement)
				{
					// never found the original channel name
					channel_status->status = STATUS_NOT_FOUND;
					found = FALSE;
				}
			}
		}
		else
			channel_status->status = STATUS_NO_CHANNELS; // that's right, nothing to find
			
		
		if( found && ( chan_desc.dimension != 1 ||
			!(	chan_desc.data_type == PF_DataType_CHAR ||
				chan_desc.data_type == PF_DataType_U_BYTE ||
				chan_desc.data_type == PF_DataType_U_SHORT ||
				chan_desc.data_type == PF_DataType_LONG  ) ) )
		{
			// sorry, we need 1D unsigned integers
			found = FALSE;
			channel_status->status = STATUS_ERROR;
		}
		
		
		// checkout the channel
		PF_ChannelChunk chan_chunk;
		
		if( found )
		{
			// get that channel
			err = cs->PF_CheckoutLayerChannel(in_data->effect_ref,
											&chan_ref,
											in_data->current_time,
											in_data->time_step,
											in_data->time_scale,
											chan_desc.data_type,
											&chan_chunk);

			if(err || !chan_chunk.dataPV)
			{
				found = FALSE;
				channel_status->status = STATUS_ERROR;
				err = PF_Err_INVALID_CALLBACK; // there was a problem, dude
			}
		}
			
		
		if( found )
		{
			size_t sizeof_chunkpix = sizeof(int);
			
			switch(chan_chunk.data_type)
			{
				case PF_DataType_CHAR:
				case PF_DataType_U_BYTE: // this is the same, right?
					sizeof_chunkpix = sizeof(char);
					break;
				
				case PF_DataType_U_SHORT:
					sizeof_chunkpix = sizeof(unsigned short);
					break;
				
				case PF_DataType_SHORT:
					sizeof_chunkpix = sizeof(short);
					break;
				
				case PF_DataType_LONG:
					sizeof_chunkpix = sizeof(int);
					break;
			}
			
			
			char *channel_row = (char *)chan_chunk.dataPV;
			char *input_row = (char *)input->data;
			char *output_row = (char *)output->data;
			
			// the origin might not be 0,0 and the ROI might not include the whole image
			// we have to figure out where we have to move our pointers to the right spot in each buffer
			// and copy only as far as we can
			
			// if the origin is negative, we move in the world, if positive, we move in the channel
			PF_Point world_move, chan_move;
			
			world_move.h = MAX(-output->origin_x, 0);
			world_move.v = MAX(-output->origin_y, 0);
			
			chan_move.h = MAX(output->origin_x, 0);
			chan_move.v = MAX(output->origin_y, 0);
			

			int copy_height = MIN(output->height - world_move.v, chan_chunk.heightL - chan_move.v);
			int copy_width = MIN(output->width - world_move.h, chan_chunk.widthL - chan_move.h);

			// move pointers to starting pixel positions
			input_row += (world_move.v * input->rowbytes) + (world_move.h * sizeof_pixel);
			output_row += (world_move.v * output->rowbytes) + (world_move.h * sizeof_pixel);
			channel_row += (chan_move.v * chan_chunk.row_bytesL) + (chan_move.h * sizeof_chunkpix);

			
			// for alpha matte, we need the RGBA there
			if(EXTRACT_display->u.pd.value == DISPLAY_ALPHA_MATTE)
				PF_COPY(input, output, NULL, NULL);
				
			
			switch(chan_desc.data_type)
			{
				case PF_DataType_CHAR:
				case PF_DataType_U_BYTE: // this is the same, right?
					err2 = CopyIDChannel(in_data,
											(char *)channel_row, chan_chunk.row_bytesL,
											(PF_PixelPtr)output_row, format, output->rowbytes,
											EXTRACT_display->u.pd.value, EXTRACT_id->u.sd.value,
											copy_width, copy_height);
					break;
				
				case PF_DataType_U_SHORT:
					err2 = CopyIDChannel(in_data,
											(unsigned short *)channel_row, chan_chunk.row_bytesL,
											(PF_PixelPtr)output_row, format, output->rowbytes,
											EXTRACT_display->u.pd.value, EXTRACT_id->u.sd.value,
											copy_width, copy_height);
					break;

				case PF_DataType_SHORT:
					err2 = CopyIDChannel(in_data,
											(short *)channel_row, chan_chunk.row_bytesL,
											(PF_PixelPtr)output_row, format, output->rowbytes,
											EXTRACT_display->u.pd.value, EXTRACT_id->u.sd.value,
											copy_width, copy_height);
					break;
				
				case PF_DataType_LONG:
					err2 = CopyIDChannel(in_data,
											(int *)channel_row, chan_chunk.row_bytesL,
											(PF_PixelPtr)output_row, format, output->rowbytes,
											EXTRACT_display->u.pd.value, EXTRACT_id->u.sd.value,
											copy_width, copy_height);
					break;
			}
			
			// check that channel back in
			err = cs->PF_CheckinLayerChannel(	in_data->effect_ref,
												&chan_ref,
												&chan_chunk);
		}
		else
			PF_COPY(input, output, NULL, NULL); // if we don't find a channel


		// to force a UI refresh after pixels have been sampled - might want to do this less often
		out_data->out_flags |= PF_OutFlag_REFRESH_UI;
	}

	// error pecking order
	if(err3)
		err = err3;
	else if(err2)
		err = err2;

	return err;
}

static PF_Err
SmartRender(
	PF_InData				*in_data,
	PF_OutData				*out_data,
	PF_SmartRenderExtra		*extra)

{

	PF_Err			err		= PF_Err_NONE,
					err2 	= PF_Err_NONE;
					
	PF_EffectWorld *input, *output;
	
	PF_ParamDef EXTRACT_data,
				EXTRACT_display,
				EXTRACT_id;

	// zero-out parameters
	AEFX_CLR_STRUCT(EXTRACT_data);
	AEFX_CLR_STRUCT(EXTRACT_display);
	AEFX_CLR_STRUCT(EXTRACT_id);
	

	// checkout input & output buffers.
	ERR(	extra->cb->checkout_layer_pixels( in_data->effect_ref, EXTRACT_INPUT, &input)	);
	ERR(	extra->cb->checkout_output(	in_data->effect_ref, &output)	);


	// bail before param checkout
	if(err)
		return err;

#define PF_CHECKOUT_PARAM_NOW( PARAM, DEST )	PF_CHECKOUT_PARAM(	in_data, (PARAM), in_data->current_time, in_data->time_step, in_data->time_scale, DEST )

	// checkout the required params
	ERR(	PF_CHECKOUT_PARAM_NOW( EXTRACT_DATA,			&EXTRACT_data )	);
	ERR(	PF_CHECKOUT_PARAM_NOW( EXTRACT_DISPLAY,			&EXTRACT_display )	);
	ERR(	PF_CHECKOUT_PARAM_NOW( EXTRACT_CHANNEL_ID,		&EXTRACT_id )	);

	ERR(	DoRender(	in_data, 
						input, 
						&EXTRACT_data, 
						&EXTRACT_display,
						&EXTRACT_id,
						out_data, 
						output) );

	// Always check in, no matter what the error condition!
	ERR2(	PF_CHECKIN_PARAM(in_data, &EXTRACT_data )	);
	ERR2(	PF_CHECKIN_PARAM(in_data, &EXTRACT_display )	);
	ERR2(	PF_CHECKIN_PARAM(in_data, &EXTRACT_id )	);


	return err;
}


DllExport	
PF_Err 
PluginMain (	
	PF_Cmd			cmd,
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output,
	void			*extra)
{
	PF_Err		err = PF_Err_NONE;
	
	try	{
		switch (cmd) {
			case PF_Cmd_ABOUT:
				err = About(in_data,out_data,params,output);
				break;
			case PF_Cmd_GLOBAL_SETUP:
				err = GlobalSetup(in_data,out_data,params,output);
				break;
			case PF_Cmd_PARAMS_SETUP:
				err = ParamsSetup(in_data,out_data,params,output);
				break;
			case PF_Cmd_SEQUENCE_SETUP:
			case PF_Cmd_SEQUENCE_RESETUP:
				err = SequenceSetup(in_data, out_data, params, output);
				break;
			case PF_Cmd_SEQUENCE_FLATTEN:
				err = SequenceFlatten(in_data, out_data, params, output);
				break;
			case PF_Cmd_SEQUENCE_SETDOWN:
				err = SequenceSetdown(in_data, out_data, params, output);
				break;
			case PF_Cmd_SMART_PRE_RENDER:
				err = PreRender(in_data, out_data, (PF_PreRenderExtra*)extra);
				break;
			case PF_Cmd_SMART_RENDER:
				err = SmartRender(in_data, out_data, (PF_SmartRenderExtra*)extra);
				break;
			case PF_Cmd_EVENT:
				err = HandleEvent(in_data, out_data, params, output, (PF_EventExtra	*)extra);
				break;
			case PF_Cmd_DO_DIALOG:
				err = DoDialog(in_data, out_data, params, output);
				break;	
			case PF_Cmd_ARBITRARY_CALLBACK:
				err = HandleArbitrary(in_data, out_data, params, output, (PF_ArbParamsExtra	*)extra);
				break;
		}
	}
	catch(PF_Err &thrown_err) { err = thrown_err; }
	catch(...) { err = PF_Err_INTERNAL_STRUCT_DAMAGED; }
	
	return err;
}
