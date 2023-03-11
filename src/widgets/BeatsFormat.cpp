/**********************************************************************

  Audacity: A Digital Audio Editor

  BeatsFormat.cpp

  Michael Papadopoulos

**********************************************************************/

#include "BeatsFormat.h"

void BeatsFormat::SetTickSizes(
   double units, double& mMajor, double& mMinor, double& mMinorMinor,
   int& mDigits, const std::any& data
) const
{
   const BeatsFormatData* beatsData = std::any_cast<BeatsFormatData>(&data);

   const double bpm = beatsData ? beatsData->bpm : 60;
   const int timeSigUpper = beatsData ? beatsData->timeSigUpper : 4;
   const int timeSigLower = beatsData ? beatsData->timeSigLower : 4;

   // Check that all data is positive
   if (!(bpm > 0 && timeSigUpper > 0 && timeSigLower > 0)) return;
   // Also check that the lower time signature is valid (power of 2)
   if(timeSigLower & (timeSigLower - 1)) return;

   if (units < .05 * (60 / bpm))
   {
      // measures
      mMajor = (60 * timeSigUpper) / (bpm * ((double)timeSigLower / 4));
      // sixteenth notes (label every quarter note)
      mMinor = 60 / (bpm * ((double)timeSigLower));
      // sixtyfourth notes
      mMinorMinor = 60 / (bpm * ((double)timeSigLower * 4));
   }
   else if (units < .1 * (60 / bpm))
   {
      // measures
      mMajor = (60 * timeSigUpper) / (bpm * ((double)timeSigLower / 4));
      // eigth notes (label every quarter note)
      mMinor = 60 / (bpm * ((double)timeSigLower / 2));
      // thirtysecondth notes
      mMinorMinor = 60 / (bpm * ((double)timeSigLower * 2));
   }
   else if (units < .4 * (60 / bpm))
   {
      // measures
      mMajor = (60 * timeSigUpper) / (bpm * ((double)timeSigLower / 4));
      // eigth notes (label every quarter note)
      mMinor = 60 / (bpm * ((double)timeSigLower / 2));
      // sixteenth notes
      mMinorMinor = 60 / (bpm * ((double)timeSigLower));
   }
   else if (units < .8 * (60 / bpm))
   {
      // measures
      mMajor = (60 * timeSigUpper) / (bpm * ((double)timeSigLower / 4));
      // quarter notes
      mMinor = 60 / (bpm * ((double)timeSigLower / 4));
      // sixteenth notes
      mMinorMinor = 60 / (bpm * ((double)timeSigLower));
   }
   else if (units < 4 * (60 / bpm))
   {
      // measures
      mMajor = (60 * timeSigUpper) / (bpm * ((double)timeSigLower / 4));
      // quarter notes
      mMinorMinor = 60 / (bpm * ((double)timeSigLower / 4));
   }
   else {
      // Introduce a scaling factor so space is maintained between
      // measures as the units increase
      int factor = std::ceil(units / 4);
      mMajor = (60 * timeSigUpper * factor) / (bpm * ((double)timeSigLower / 8));
      mMinor = (60 * factor) / (bpm * ((double)timeSigLower / 16));
      mMinorMinor = (60 * factor) / (bpm * ((double)timeSigLower / 4));
   }

   mDigits = 0;
}

void BeatsFormat::SetLabelString(
   wxString& s, double d, double units, double mMinor, int mDigits, TickType tickType,
   const std::any& data
) const
{
   if (d < 0) {
      return;
   }
   const BeatsFormatData* beatsData = std::any_cast<BeatsFormatData>(&data);

   const double bpm = beatsData ? beatsData->bpm : 0;
   const int timeSigUpper = beatsData ? beatsData->timeSigUpper : 0;
   const int timeSigLower = beatsData ? beatsData->timeSigLower : 0;

   double val = (bpm * ((double)timeSigLower / 4) * d) / (60 * timeSigUpper);
   double beatApprox = (val - floor(val)) * timeSigUpper + 1;
   int beat = round(beatApprox);

   // Don't add decimal if it's a major tick or is on the beat
   // Segment by distance with units
   if (units < .5 * (60 / bpm))
   {
      if (tickType == RulerFormat::t_major) {
         s.Printf(wxT("%d"), (int)round(val + 1));
      }
      else if (tickType == RulerFormat::t_minor && abs(beat - beatApprox) < 1.0e-5f) {
         s.Printf(wxT("%d.%d"), (int)floor(val + 1), (int)beat);
      }
   }
   else if (units < 1 * (60 / bpm))
   {
      if (tickType == RulerFormat::t_major || tickType == RulerFormat::t_minor && beat == 1) {
         s.Printf(wxT("%d"), (int)round(val + 1));
      }
      else if (tickType == RulerFormat::t_minor) {
         s.Printf(wxT("%d.%d"), (int)floor(val + 1), (int)beat);
      }
   }
   else {
      if (tickType == RulerFormat::t_major) {
         s.Printf(wxT("%d"), (int)round(val + 1));
      }
   }
}

BeatsFormat::~BeatsFormat() = default;
