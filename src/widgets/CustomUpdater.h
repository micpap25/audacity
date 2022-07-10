/**********************************************************************

  Audacity: A Digital Audio Editor

  CustomUpdater.h

  Dominic Mazzoni

**********************************************************************/

#ifndef __AUDACITY_CUSTOM_UPDATER__
#define __AUDACITY_CUSTOM_UPDATER__

#include "Ruler.h"
#include "Updater.h"

struct Ruler::CustomUpdater : public Ruler::Updater {
   explicit CustomUpdater(const Ruler& ruler, const ZoomInfo* z)
      : Updater{ ruler, NULL }
   {}

   void Update(
      wxDC& dc, const Envelope* envelope,
      UpdateOutputs& allOutputs
   ) const override;
};

void Ruler::CustomUpdater::Update(
   wxDC& dc, const Envelope* envelope, UpdateOutputs& allOutputs) const
{
   TickOutputs majorOutputs{
      allOutputs.majorLabels, allOutputs.bits, allOutputs.box };

   // SET PARAMETER IN MCUSTOM CASE
   // Works only with major labels

   int numLabel = allOutputs.majorLabels.size();

   for (int i = 0; (i < numLabel) && (i <= mLength); ++i)
      TickCustom(dc, i, mFonts.major, majorOutputs);

   BoxAdjust(allOutputs);
}
