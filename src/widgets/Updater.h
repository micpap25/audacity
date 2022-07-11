/**********************************************************************

  Audacity: A Digital Audio Editor

  Updater.h

  Dominic Mazzoni

**********************************************************************/

#ifndef __AUDACITY_UPDATER__
#define __AUDACITY_UPDATER__

#include "Ruler.h"

class Ruler::Label;

struct Updater {
private:
   using Labels = std::vector<Ruler::Label>;
   using Bits = std::vector< bool >;

public:

   const Ruler& mRuler;
   const ZoomInfo* zoomInfo;

   explicit Updater(const Ruler& ruler, const ZoomInfo* z)
      : mRuler{ ruler }
      , zoomInfo{ z }
   {}
   ~Updater() {}

   const double mDbMirrorValue = mRuler.mDbMirrorValue;
   const int mLength = mRuler.mLength;
   const Ruler::RulerFormat mFormat = mRuler.mFormat;
   const TranslatableString mUnits = mRuler.mUnits;

   const int mLeft = mRuler.mLeft;
   const int mTop = mRuler.mTop;
   const int mBottom = mRuler.mBottom;
   const int mRight = mRuler.mRight;

   const int mSpacing = mRuler.mSpacing;
   const int mOrientation = mRuler.mOrientation;
   const bool mFlip = mRuler.mFlip;

   const bool mCustom = mRuler.mCustom;
   const Ruler::Fonts& mFonts = *mRuler.mpFonts;
   const bool mLog = mRuler.mLog;
   const double mHiddenMin = mRuler.mHiddenMin;
   const double mHiddenMax = mRuler.mHiddenMax;
   const bool mLabelEdges = mRuler.mLabelEdges;
   const double mMin = mRuler.mMin;
   const double mMax = mRuler.mMax;
   const int mLeftOffset = mRuler.mLeftOffset;
   const NumberScale mNumberScale = mRuler.mNumberScale;



   struct TickOutputs { Labels& labels; Bits& bits; wxRect& box; };
   struct UpdateOutputs {
      Labels& majorLabels, & minorLabels, & minorMinorLabels;
      Bits& bits;
      wxRect& box;
   };

   struct TickSizes;

   bool Tick(wxDC& dc,
      int pos, double d, const TickSizes& tickSizes, wxFont font,
      TickOutputs outputs
   ) const;

   // Another tick generator for custom ruler case (noauto) .
   bool TickCustom(wxDC& dc, int labelIdx, wxFont font,
      TickOutputs outputs
   ) const;

   void BoxAdjust(
      UpdateOutputs& allOutputs
   )
      const;

   virtual void Update(
      wxDC& dc, const Envelope* envelope,
      UpdateOutputs& allOutputs
   )// Envelope *speedEnv, long minSpeed, long maxSpeed )
      const = 0;
};

#endif //define __AUDACITY_UPDATER__
