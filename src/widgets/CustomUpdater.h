﻿/**********************************************************************

  Audacity: A Digital Audio Editor

  CustomUpdater.h

  Dominic Mazzoni
  Michael Papadopoulos split from Ruler.h

**********************************************************************/

#ifndef __AUDACITY_CUSTOM_UPDATER__
#define __AUDACITY_CUSTOM_UPDATER__

#include "RulerUpdater.h"


struct CustomUpdaterData { const RulerUpdater::Labels majorLabels, minorLabels, minorMinorLabels; };

class CustomUpdater : public RulerUpdater {
public:
   using RulerUpdater::RulerUpdater;
   virtual ~CustomUpdater() override = 0;

   void Update(
      wxDC& dc, const Envelope* envelope,
      UpdateOutputs& allOutputs, const RulerStruct& context, const std::any& data
   ) const final override;

protected:
   virtual bool TickCustom(wxDC& dc, int labelIdx, wxFont font,
      TickOutputs outputs,
      const RulerStruct& context
   ) const = 0;
};

#endif
