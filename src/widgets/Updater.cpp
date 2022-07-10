/**********************************************************************

  Audacity: A Digital Audio Editor

  Updater.cpp

  Dominic Mazzoni

*******************************************************************//**

\class Updater
\brief Used to update a Ruler.

  This is a pure virtual class which sets how a ruler will generate
  its values.

  It supports three subclasses: LinearUpdater, LogarithmicUpdater,
  and CustomUpdater

*//******************************************************************/

#include "Updater.h"
#include "Ruler.h"

struct Ruler::Updater::TickOutputs { Labels& labels; Bits& bits; wxRect& box; };
struct Ruler::Updater::UpdateOutputs {
   Labels& majorLabels, & minorLabels, & minorMinorLabels;
   Bits& bits;
   wxRect& box;
};


void Ruler::Updater::BoxAdjust(
   UpdateOutputs& allOutputs
)
const
{
   int displacementx = 0, displacementy = 0;
   auto& box = allOutputs.box;
   if (!mFlip) {
      if (mOrientation == wxHORIZONTAL) {
         int d = mTop + box.GetHeight() + 5;
         box.Offset(0, d);
         box.Inflate(0, 5);
         displacementx = 0;
         displacementy = d;
      }
      else {
         int d = mLeft - box.GetLeft() + 5;
         box.Offset(d, 0);
         box.Inflate(5, 0);
         displacementx = d;
         displacementy = 0;
      }
   }
   else {
      if (mOrientation == wxHORIZONTAL) {
         box.Inflate(0, 5);
         displacementx = 0;
         displacementy = 0;
      }
   }
   auto update = [=](Label& label) {
      label.lx += displacementx;
      label.ly += displacementy;
   };
   for (auto& label : allOutputs.majorLabels)
      update(label);
   for (auto& label : allOutputs.minorLabels)
      update(label);
   for (auto& label : allOutputs.minorMinorLabels)
      update(label);
}

bool Ruler::Updater::Tick(wxDC& dc,
   int pos, double d, const TickSizes& tickSizes, wxFont font,
   // in/out:
   TickOutputs outputs) const
{
   // Bug 521.  dB view for waveforms needs a 2-sided scale.
   if ((mDbMirrorValue > 1.0) && (-d > mDbMirrorValue))
      d = -2 * mDbMirrorValue - d;

   // FIXME: We don't draw a tick if off end of our label arrays
   // But we shouldn't have an array of labels.
   if (outputs.labels.size() >= mLength)
      return false;

   Label lab;
   lab.value = d;
   lab.pos = pos;
   lab.text = tickSizes.LabelString(d, mFormat, mUnits);

   const auto result = MakeTick(
      lab,
      dc, font,
      outputs.bits,
      mLeft, mTop, mSpacing, mFonts.lead,
      mFlip,
      mOrientation);

   auto& rect = result.first;
   outputs.box.Union(rect);
   outputs.labels.emplace_back(result.second);
   return !rect.IsEmpty();
}

bool Ruler::Updater::TickCustom(wxDC& dc, int labelIdx, wxFont font,
   // in/out:
   TickOutputs outputs) const
{
   // FIXME: We don't draw a tick if of end of our label arrays
   // But we shouldn't have an array of labels.
   if (labelIdx >= outputs.labels.size())
      return false;

   //This should only used in the mCustom case

   Label lab;
   lab.value = 0.0;

   const auto result = MakeTick(
      lab,

      dc, font,
      outputs.bits,
      mLeft, mTop, mSpacing, mFonts.lead,
      mFlip,
      mOrientation);

   auto& rect = result.first;
   outputs.box.Union(rect);
   outputs.labels[labelIdx] = (result.second);
   return !rect.IsEmpty();
}
