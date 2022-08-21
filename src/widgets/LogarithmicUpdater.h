﻿/**********************************************************************

  Audacity: A Digital Audio Editor

  LogarithmicUpdater.h

  Dominic Mazzoni

**********************************************************************/

#ifndef __AUDACITY_LOGARITHMIC_UPDATER__
#define __AUDACITY_LOGARITHMIC_UPDATER__

#include "GeneratedUpdater.h"

class LogarithmicUpdater final : public GeneratedUpdater {
public:
   using GeneratedUpdater::GeneratedUpdater;
   ~LogarithmicUpdater() override;

   void Update(
      wxDC& dc, const Envelope* envelope,
      UpdateOutputs& allOutputs, const RulerStruct& context, const std::any& data
   ) const override;
};

#endif

