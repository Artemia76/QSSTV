/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2004
 *
 * Author(s):
 *	Volker Fischer
 * 
 * Description:
 *
 ******************************************************************************
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
\******************************************************************************/

#include "Utilities.h"
#include <sstream>
#include <cstring>
#if defined(_WIN32)
# ifdef HAVE_SETUPAPI
#  ifndef INITGUID
#   define INITGUID 1
#  endif
#  include <windows.h>
#  include <setupapi.h>
#  if defined(_MSC_VER) && (_MSC_VER < 1400) || defined(__MINGW32__)
    DEFINE_GUID(GUID_DEVINTERFACE_COMPORT, 0x86e0d1e0L, 0x8089, 
    0x11d0, 0x9c, 0xe4, 0x08, 0x00, 0x3e, 0x30, 0x1f, 0x73);
#  endif
# endif
#elif defined(__APPLE__)
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/serial/IOSerialKeys.h>
#endif

/* Implementation *************************************************************/
/******************************************************************************\
* Signal level meter                                                           *
\******************************************************************************/
//void
//CSignalLevelMeter::Update(const _REAL rVal)
//{
//	/* Search for maximum. Decrease this max with time */
//	/* Decrease max with time */
//	if (rCurLevel >= METER_FLY_BACK)
//		rCurLevel -= METER_FLY_BACK;
//	else
//	{
//		if ((rCurLevel <= METER_FLY_BACK) && (rCurLevel > 1))
//			rCurLevel -= 2;
//	}

//	/* Search for max */
//	const _REAL rCurAbsVal = Abs(rVal);
//	if (rCurAbsVal > rCurLevel)
//		rCurLevel = rCurAbsVal;
//}

//void
//CSignalLevelMeter::Update(const CVector < _REAL > vecrVal)
//{
//	/* Do the update for entire vector */
//	const int iVecSize = vecrVal.Size();
//	for (int i = 0; i < iVecSize; i++)
//		Update(vecrVal[i]);
//}

//void
//CSignalLevelMeter::Update(const CVector < _SAMPLE > vecsVal)
//{
//	/* Do the update for entire vector, convert to real */
//	const int iVecSize = vecsVal.Size();
//	for (int i = 0; i < iVecSize; i++)
//		Update((_REAL) vecsVal[i]);
//}

//_REAL CSignalLevelMeter::Level()
//{
//	const _REAL
//		rNormMicLevel = rCurLevel / _MAXSHORT;

//	/* Logarithmic measure */
//	if (rNormMicLevel > 0)
//		return 20.0 * log10(rNormMicLevel);
//	else
//		return RET_VAL_LOG_0;
//}

/******************************************************************************\
* Bandpass filter                                                              *
\******************************************************************************/
void
CDRMBandpassFilt::Process(CVector < _COMPLEX > &veccData)
{
	int i;

	/* Copy CVector data in CMatlibVector */
	for (i = 0; i < iBlockSize; i++)
		cvecDataTmp[i] = veccData[i];

	/* Apply FFT filter */
	cvecDataTmp =
		CComplexVector(FftFilt
					   (cvecB, Real(cvecDataTmp), rvecZReal, FftPlanBP),
					   FftFilt(cvecB, Imag(cvecDataTmp), rvecZImag,
							   FftPlanBP));

	/* Copy CVector data in CMatlibVector */
	for (i = 0; i < iBlockSize; i++)
		veccData[i] = cvecDataTmp[i];
}

void
CDRMBandpassFilt::Init(const int iNewBlockSize, const _REAL rOffsetHz,
					   const ESpecOcc eSpecOcc, const EFiltType eNFiTy)
{
	CReal rMargin = 0.0;

	/* Set internal parameter */
	iBlockSize = iNewBlockSize;

	/* Init temporary vector */
	cvecDataTmp.Init(iBlockSize);

	/* Choose correct filter for chosen DRM bandwidth. Also, adjust offset
	   frequency for different modes. E.g., 5 kHz mode is on the right side
	   of the DC frequency */
	CReal rNormCurFreqOffset = rOffsetHz / SOUNDCRD_SAMPLE_RATE;
	/* Band-pass filter bandwidth */
	CReal rBPFiltBW = ((CReal) 10000.0 + rMargin) / SOUNDCRD_SAMPLE_RATE;

	/* Negative margin for receiver filter for better interferer rejection */
	if (eNFiTy == FT_TRANSMITTER)
		rMargin = (CReal) 0.0;	/* Hz was 300 */
	else
		rMargin = (CReal) 0.0;	/* Hz */

	switch (eSpecOcc)
	{
	case SO_0:
		rBPFiltBW = ((CReal) 2000.0 + rMargin) / SOUNDCRD_SAMPLE_RATE;   // pa0mbo was 4500.0 moet zijn 2250?

		/* Completely on the right side of DC */
		rNormCurFreqOffset =
			(rOffsetHz + (CReal) 1100.0) / SOUNDCRD_SAMPLE_RATE;    // pa0mbo
//			( (CReal) 7100.0  ) / SOUNDCRD_SAMPLE_RATE;    // pa0mbo
		         // printf("util Bandpass filt init SO_0  BW %g rNormOffs %g\n", rBPFiltBW, rNormCurFreqOffset);
		break;

	case SO_1:
		rBPFiltBW = ((CReal) 2500.0 + rMargin) / SOUNDCRD_SAMPLE_RATE;  // pa0mbo was 5000.0 moet zijn 2500  ?

		/* Completely on the right side of DC */
		rNormCurFreqOffset =
			(rOffsetHz + (CReal) 1250.0) / SOUNDCRD_SAMPLE_RATE;   // pa0mbo
//			( (CReal) 7240.0 )  / SOUNDCRD_SAMPLE_RATE;   // pa0mbo
		// printf("util drmbandpass SO_1   BW = %g , Offset = %g \n", rBPFiltBW,  rNormCurFreqOffset);
		break;

	case SO_2:
		rBPFiltBW = ((CReal) 9000.0 + rMargin) / SOUNDCRD_SAMPLE_RATE;

		/* Centered */
		rNormCurFreqOffset = rOffsetHz / SOUNDCRD_SAMPLE_RATE;
		break;

	case SO_3:
		rBPFiltBW = ((CReal) 10000.0 + rMargin) / SOUNDCRD_SAMPLE_RATE;

		/* Centered */
		rNormCurFreqOffset = rOffsetHz / SOUNDCRD_SAMPLE_RATE;
		break;

	case SO_4:
		rBPFiltBW = ((CReal) 18000.0 + rMargin) / SOUNDCRD_SAMPLE_RATE;

		/* Main part on the right side of DC */
		rNormCurFreqOffset =
			(rOffsetHz + (CReal) 4500.0) / SOUNDCRD_SAMPLE_RATE;
		break;

	case SO_5:
		rBPFiltBW = ((CReal) 20000.0 + rMargin) / SOUNDCRD_SAMPLE_RATE;

		/* Main part on the right side of DC */
		rNormCurFreqOffset =
			(rOffsetHz + (CReal) 5000.0) / SOUNDCRD_SAMPLE_RATE;
		break;
	}
         // printf("Init bandpass BW = %g rNorm %g \n", rBPFiltBW, rNormCurFreqOffset); 
	/* FFT plan is initialized with the long length */
	FftPlanBP.Init(iBlockSize * 2);

	/* State memory (init with zeros) and data vector */
	rvecZReal.Init(iBlockSize, (CReal) 0.0);
	rvecZImag.Init(iBlockSize, (CReal) 0.0);
	rvecDataReal.Init(iBlockSize);
	rvecDataImag.Init(iBlockSize);

	/* "+ 1" because of the Nyquist frequency (filter in frequency domain) */
	cvecB.Init(iBlockSize + 1);

	/* Actual filter design */
	CRealVector vecrFilter(iBlockSize);
	vecrFilter = FirLP(rBPFiltBW, Nuttallwin(iBlockSize));

	/* Copy actual filter coefficients. It is important to initialize the
	   vectors with zeros because we also do a zero-padding */
	CRealVector rvecB(2 * iBlockSize, (CReal) 0.0);

	/* Modulate filter to shift it to the correct IF frequency */
	for (int i = 0; i < iBlockSize; i++)
	{
		rvecB[i] =
			vecrFilter[i] * Cos((CReal) 2.0 * crPi * rNormCurFreqOffset * i);
	}

	/* Transformation in frequency domain for fft filter */
	cvecB = rfft(rvecB, FftPlanBP);
        /* debugging pa0mbo 
        for (i=0; i < iBlockSize; i++)
        {
           printf(" vecrFilter[%d] is  %g cvecB[%d] real is %g \n", i, vecrFilter[i], i, cvecB[i].real());
        }
        */

	// printf("end of init BPfilt, iBlockSize %d rBPFBW %g rOffsetHz %g i rNormFreqoff %g\n",
			// iBlockSize, rBPFiltBW, rOffsetHz, rNormCurFreqOffset ) ;
	// printf("Soundcard sample rate is %d \n", SOUNDCRD_SAMPLE_RATE);
}

/******************************************************************************\
* Modified Julian Date                                                         *
\******************************************************************************/
void
CModJulDate::Set(const uint32_t iModJulDate)
{
	uint32_t iZ, iA, iAlpha, iB, iC, iD, iE;
  _REAL rJulDate;

	/* Definition of the Modified Julian Date */
	rJulDate = (_REAL) iModJulDate + 2400000.5;

	/* Get "real" date out of Julian Date
	   (Taken from "http://mathforum.org/library/drmath/view/51907.html") */
	// 1. Add .5 to the JD and let Z = integer part of (JD+.5) and F the
	// fractional part F = (JD+.5)-Z
	iZ = (uint32_t) (rJulDate + (_REAL) 0.5);
//	rF = (rJulDate + (_REAL) 0.5) - iZ;

	// 2. If Z < 2299161, take A = Z
	// If Z >= 2299161, calculate alpha = INT((Z-1867216.25)/36524.25)
	// and A = Z + 1 + alpha - INT(alpha/4).
	if (iZ < 2299161)
		iA = iZ;
	else
	{
		iAlpha = (int) (((_REAL) iZ - (_REAL) 1867216.25) / (_REAL) 36524.25);
		iA = iZ + 1 + iAlpha - (int) ((_REAL) iAlpha / (_REAL) 4.0);
	}

	// 3. Then calculate:
	// B = A + 1524
	// C = INT( (B-122.1)/365.25)
	// D = INT( 365.25*C )
	// E = INT( (B-D)/30.6001 )
	iB = iA + 1524;
	iC = (int) (((_REAL) iB - (_REAL) 122.1) / (_REAL) 365.25);
	iD = (int) ((_REAL) 365.25 * iC);
	iE = (int) (((_REAL) iB - iD) / (_REAL) 30.6001);

	// The day of the month dd (with decimals) is:
	// dd = B - D - INT(30.6001*E) + F
	iDay = iB - iD - (int) ((_REAL) 30.6001 * iE);	// + rF;

	// The month number mm is:
	// mm = E - 1, if E < 13.5
	// or
	// mm = E - 13, if E > 13.5
	if ((_REAL) iE < 13.5)
		iMonth = iE - 1;
	else
		iMonth = iE - 13;

	// The year yyyy is:
	// yyyy = C - 4716   if m > 2.5
	// or
	// yyyy = C - 4715   if m < 2.5
	if ((_REAL) iMonth > 2.5)
		iYear = iC - 4716;
	else
		iYear = iC - 4715;
}




