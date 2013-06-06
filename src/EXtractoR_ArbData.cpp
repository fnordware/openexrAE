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
//	mmm...arbitrary
//
//

#include "EXtractoR.h"

#ifndef __MACH__
#include <assert.h>
#endif


PF_Err
ArbNewDefault(PF_InData *in_data, PF_OutData *out_data,
	void				*refconPV,
	PF_ArbitraryH		*arbPH)
{
	PF_Err err = PF_Err_NONE;
	
	if(arbPH)
	{
		*arbPH = PF_NEW_HANDLE( sizeof(ArbitraryData) );
		
		if(*arbPH)
		{
			ArbitraryData *arb_data = (ArbitraryData *)PF_LOCK_HANDLE(*arbPH);
			
			// set up defaults
			arb_data->version = CURRENT_ARB_VERSION;
			arb_data->single_channel = FALSE;
			
			arb_data->red.action = DO_EXTRACT;
			arb_data->red.index = 0;
			strcpy(arb_data->red.name, "R");

			arb_data->green.action = DO_EXTRACT;
			arb_data->green.index = 0;
			strcpy(arb_data->green.name, "G");

			arb_data->blue.action = DO_EXTRACT;
			arb_data->blue.index = 0;
			strcpy(arb_data->blue.name, "B");

			arb_data->alpha.action = DO_COPY;
			arb_data->alpha.index = 0;
			strcpy(arb_data->alpha.name, "(copy)");
			
			PF_UNLOCK_HANDLE(*arbPH);
		}
	}
	
	return err;
}


static PF_Err
ArbDispose(PF_InData *in_data, PF_OutData *out_data,
	void				*refconPV,
	PF_ArbitraryH		arbH)
{
	if(arbH)
		PF_DISPOSE_HANDLE(arbH);
	
	return PF_Err_NONE;
}


static void
CopyArbData(ArbitraryData *out_arb_data, ArbitraryData *in_arb_data)
{
	// copy contents
	out_arb_data->single_channel = in_arb_data->single_channel;
	
	out_arb_data->red.action = in_arb_data->red.action;
	out_arb_data->red.index = in_arb_data->red.index;
	strcpy(out_arb_data->red.name, in_arb_data->red.name);

	out_arb_data->green.action = in_arb_data->green.action;
	out_arb_data->green.index = in_arb_data->green.index;
	strcpy(out_arb_data->green.name, in_arb_data->green.name);

	out_arb_data->blue.action = in_arb_data->blue.action;
	out_arb_data->blue.index = in_arb_data->blue.index;
	strcpy(out_arb_data->blue.name, in_arb_data->blue.name);

	out_arb_data->alpha.action = in_arb_data->alpha.action;
	out_arb_data->alpha.index = in_arb_data->alpha.index;
	strcpy(out_arb_data->alpha.name, in_arb_data->alpha.name);
}


static PF_Err
ArbCopy(PF_InData *in_data, PF_OutData *out_data,
	void				*refconPV,
	PF_ArbitraryH		src_arbH,
	PF_ArbitraryH 		*dst_arbPH)
{
	PF_Err 	err 	= PF_Err_NONE;
	
	if(src_arbH && dst_arbPH)
	{
		// allocate using the creation function
		err = ArbNewDefault(in_data, out_data, refconPV, dst_arbPH);
		
		if(!err)
		{
			ArbitraryData *in_arb_data = (ArbitraryData *)PF_LOCK_HANDLE(src_arbH),
							*out_arb_data = (ArbitraryData *)PF_LOCK_HANDLE(*dst_arbPH);
			
			CopyArbData(out_arb_data, in_arb_data);
			
			PF_UNLOCK_HANDLE(src_arbH);
			PF_UNLOCK_HANDLE(*dst_arbPH);
		}
	}
	
	return err;
}


static PF_Err
ArbFlatSize(PF_InData *in_data, PF_OutData *out_data,
	void				*refconPV,
	PF_ArbitraryH		arbH,
	A_u_long			*flat_data_sizePLu)
{
	// flat is the same size as inflated
	if(arbH)
		*flat_data_sizePLu = PF_GET_HANDLE_SIZE(arbH);
	
	return PF_Err_NONE;
}


#ifndef SWAP_LONG
#define SWAP_LONG(a)		((a >> 24) | ((a >> 8) & 0xff00) | ((a << 8) & 0xff0000) | (a << 24))
#endif

static void 
SwapArbData(ArbitraryData *arb_data)
{
	SWAP_LONG(arb_data->red.index);
	SWAP_LONG(arb_data->green.index);
	SWAP_LONG(arb_data->blue.index);
	SWAP_LONG(arb_data->alpha.index);
}


static PF_Err
ArbFlatten(PF_InData *in_data, PF_OutData *out_data,
	void				*refconPV,
	PF_ArbitraryH		arbH,
	A_u_long			buf_sizeLu,
	void				*flat_dataPV)
{
	PF_Err 	err 	= PF_Err_NONE;
	
	if(arbH && flat_dataPV)
	{
		// they provide the buffer, we just move data
		ArbitraryData *in_arb_data = (ArbitraryData *)PF_LOCK_HANDLE(arbH),
						*out_arb_data = (ArbitraryData *)flat_dataPV;

		assert(buf_sizeLu >= PF_GET_HANDLE_SIZE(arbH));
	
		CopyArbData(out_arb_data, in_arb_data);
		
	#ifdef AE_LITTLE_ENDIAN
		// we want to store data in the PPC Mac style, for old time's sake
		SwapArbData(out_arb_data);
	#endif
	
		PF_UNLOCK_HANDLE(arbH);
	}
	
	return err;
}


static PF_Err
ArbUnFlatten(PF_InData *in_data, PF_OutData *out_data,
	void				*refconPV,
	A_u_long			buf_sizeLu,
	const void			*flat_dataPV,
	PF_ArbitraryH		*arbPH)
{
	PF_Err 	err 	= PF_Err_NONE;
	
	if(arbPH && flat_dataPV)
	{
		// they provide a flat buffer, we have to make the handle (using the default function)
		err = ArbNewDefault(in_data, out_data, refconPV, arbPH);
		
		if(!err && *arbPH)
		{
			ArbitraryData *in_arb_data = (ArbitraryData *)flat_dataPV,
							*out_arb_data = (ArbitraryData *)PF_LOCK_HANDLE(*arbPH);

			assert(buf_sizeLu <= PF_GET_HANDLE_SIZE(*arbPH));
		
			CopyArbData(out_arb_data, in_arb_data);
			
		#ifdef AE_LITTLE_ENDIAN
			// swap bytes back to platform style
			SwapArbData(out_arb_data);
		#endif
		
			PF_UNLOCK_HANDLE(*arbPH);
		}
	}
	
	return err;
}

static PF_Err
ArbInterpolate(PF_InData *in_data, PF_OutData *out_data,
	void				*refconPV,
	PF_ArbitraryH		left_arbH,
	PF_ArbitraryH		right_arbH,
	PF_FpLong			tF,
	PF_ArbitraryH		*interpPH)
{
	PF_Err 	err 	= PF_Err_NONE;
	
	assert(FALSE); // we shouldn't be doing this in EXtractoR - we said we didn't interpolate
	
	if(left_arbH && right_arbH && interpPH)
	{
		// allocate using our own func
		err = ArbNewDefault(in_data, out_data, refconPV, interpPH);
		
		if(!err && *interpPH)
		{
			// we're just going to copy the left_data
			ArbitraryData *in_arb_data = (ArbitraryData *)PF_LOCK_HANDLE(left_arbH),
							*out_arb_data = (ArbitraryData *)PF_LOCK_HANDLE(*interpPH);
			
			CopyArbData(out_arb_data, in_arb_data);
			
			PF_UNLOCK_HANDLE(left_arbH);
			PF_UNLOCK_HANDLE(*interpPH);
		}
	}
	
	return err;
}


static PF_Err
ArbCompare(PF_InData *in_data, PF_OutData *out_data,
	void				*refconPV,
	PF_ArbitraryH		a_arbH,
	PF_ArbitraryH		b_arbH,
	PF_ArbCompareResult	*compareP)
{
	PF_Err 	err 	= PF_Err_NONE;
	
	if(a_arbH && b_arbH)
	{
		ArbitraryData *a_data = (ArbitraryData *)PF_LOCK_HANDLE(a_arbH),
						*b_data = (ArbitraryData *)PF_LOCK_HANDLE(b_arbH);
		
		if(
			a_data->single_channel == b_data->single_channel		&&
			
			a_data->red.action == b_data->red.action				&&
			!strcmp(a_data->red.name, b_data->red.name)				&&

			a_data->green.action == b_data->green.action			&&
			!strcmp(a_data->green.name, b_data->green.name)			&&

			a_data->blue.action == b_data->blue.action				&&
			!strcmp(a_data->blue.name, b_data->blue.name)			&&

			a_data->alpha.action == b_data->alpha.action			&&
			!strcmp(a_data->alpha.name, b_data->alpha.name)
		)
			*compareP = PF_ArbCompare_EQUAL;
		else
			*compareP = PF_ArbCompare_NOT_EQUAL;
		
		
		PF_UNLOCK_HANDLE(a_arbH);
		PF_UNLOCK_HANDLE(b_arbH);
	}
	
	return err;
}


PF_Err 
HandleArbitrary(
	PF_InData			*in_data,
	PF_OutData			*out_data,
	PF_ParamDef			*params[],
	PF_LayerDef			*output,
	PF_ArbParamsExtra	*extra)
{
	PF_Err 	err 	= PF_Err_NONE;
	
	if(extra->id == ARBITRARY_DATA_ID)
	{
		switch(extra->which_function)
		{
			case PF_Arbitrary_NEW_FUNC:
				err = ArbNewDefault(in_data, out_data, extra->u.new_func_params.refconPV, extra->u.new_func_params.arbPH);
				break;
			case PF_Arbitrary_DISPOSE_FUNC:
				err = ArbDispose(in_data, out_data, extra->u.dispose_func_params.refconPV, extra->u.dispose_func_params.arbH);
				break;
			case PF_Arbitrary_COPY_FUNC:
				err = ArbCopy(in_data, out_data, extra->u.copy_func_params.refconPV, extra->u.copy_func_params.src_arbH, extra->u.copy_func_params.dst_arbPH);
				break;
			case PF_Arbitrary_FLAT_SIZE_FUNC:
				err = ArbFlatSize(in_data, out_data, extra->u.flat_size_func_params.refconPV, extra->u.flat_size_func_params.arbH, extra->u.flat_size_func_params.flat_data_sizePLu);
				break;
			case PF_Arbitrary_FLATTEN_FUNC:
				err = ArbFlatten(in_data, out_data, extra->u.flatten_func_params.refconPV, extra->u.flatten_func_params.arbH, extra->u.flatten_func_params.buf_sizeLu, extra->u.flatten_func_params.flat_dataPV);
				break;
			case PF_Arbitrary_UNFLATTEN_FUNC:
				err = ArbUnFlatten(in_data, out_data, extra->u.unflatten_func_params.refconPV, extra->u.unflatten_func_params.buf_sizeLu, extra->u.unflatten_func_params.flat_dataPV, extra->u.unflatten_func_params.arbPH);
				break;
			case PF_Arbitrary_INTERP_FUNC:
				err = ArbInterpolate(in_data, out_data, extra->u.interp_func_params.refconPV, extra->u.interp_func_params.left_arbH, extra->u.interp_func_params.right_arbH, extra->u.interp_func_params.tF, extra->u.interp_func_params.interpPH);
				break;
			case PF_Arbitrary_COMPARE_FUNC:
				err = ArbCompare(in_data, out_data, extra->u.compare_func_params.refconPV, extra->u.compare_func_params.a_arbH, extra->u.compare_func_params.b_arbH, extra->u.compare_func_params.compareP);
				break;
			// these are necessary for copying and pasting keyframes
			// for now, we better not be called to do this
			case PF_Arbitrary_PRINT_SIZE_FUNC:
				assert(FALSE); //err = ArbPrintSize(in_data, out_data, extra->u.print_size_func_params.refconPV, extra->u.print_size_func_params.arbH, extra->u.print_size_func_params.print_sizePLu);
				break;
			case PF_Arbitrary_PRINT_FUNC:
				assert(FALSE); //err = ArbPrint(in_data, out_data, extra->u.print_func_params.refconPV, extra->u.print_func_params.print_flags, extra->u.print_func_params.arbH, extra->u.print_func_params.print_sizeLu, extra->u.print_func_params.print_bufferPC);
				break;
			case PF_Arbitrary_SCAN_FUNC:
				assert(FALSE); //err = ArbScan(in_data, out_data, extra->u.scan_func_params.refconPV, extra->u.scan_func_params.bufPC, extra->u.scan_func_params.bytes_to_scanLu, extra->u.scan_func_params.arbPH);
				break;
		}
	}
	
	
	return err;
}
