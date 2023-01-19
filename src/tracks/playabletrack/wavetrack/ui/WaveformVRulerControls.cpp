/**********************************************************************

Audacity: A Digital Audio Editor

WaveformVRulerControls.cpp

Paul Licameli split from WaveTrackVRulerControls.cpp

**********************************************************************/

#include "WaveformVRulerControls.h"

#include "WaveformVZoomHandle.h"
#include "WaveTrackVRulerControls.h"

#include "NumberScale.h"
#include "ProjectHistory.h"
#include "../../../../RefreshCode.h"
#include "../../../../TrackPanelMouseEvent.h"
#include "../../../../UIHandle.h"
#include "../../../../WaveTrack.h"
#include "../../../../prefs/WaveformSettings.h"
#include "../../../../widgets/Ruler.h"
#include "../../../../widgets/LinearUpdater.h"
#include "../../../../widgets/CustomUpdaterValue.h"
#include "../../../../widgets/RealFormat.h"

WaveformVRulerControls::~WaveformVRulerControls() = default;

// These are doubles beacuse of the type of value in Label,
// but for the purpose of labelling the linear dB waveform ruler,
// these should always be integer numbers.
using LinearDBValues = std::vector<double>;

static LinearDBValues majorValues{}, minorValues{}, minorMinorValues{};

void RegenerateLinearDBValues(int dBRange, float min, float max)
{
   majorValues.clear();
   minorValues.clear();
   minorMinorValues.clear();

   majorValues.push_back(0);
   majorValues.push_back(-dBRange);
   majorValues.push_back(2 * -dBRange);

   const double EPSILON = .01;
   const double CHECK = 3 *(max - min) / 10;

   for (double major = 0.1; major <= 2 + EPSILON; major += .1) {
      if (fabs(1 - major) + EPSILON >= CHECK) {
         double val = std::round(major * 10) / 10;
         if (fabs(major - 1) > EPSILON)
            majorValues.push_back(std::trunc(-dBRange * val));
      }
   }
   for (double minor = 0.05; minor <= 1.95 + EPSILON; minor += .1) {
      if (fabs(1 - minor) + EPSILON >= CHECK) {
         double val = std::round(minor * 100) / 100;
         minorValues.push_back(std::trunc(-dBRange * val));
      }
   }
   for (int minorMinor = 0; minorMinor <= 2 * dBRange; minorMinor++) {
      if (fabs(dBRange - minorMinor) + EPSILON >= dBRange * CHECK) {
         if ((minorMinor % (int)std::round(dBRange / 20)) != 0) {
            minorMinorValues.push_back(-minorMinor);
         }
      }
   }

}

std::vector<UIHandlePtr> WaveformVRulerControls::HitTest(
   const TrackPanelMouseState &st,
   const AudacityProject *pProject)
{
   std::vector<UIHandlePtr> results;

   if ( st.state.GetX() <= st.rect.GetRight() - kGuard ) {
      auto pTrack = FindTrack()->SharedPointer<WaveTrack>(  );
      if (pTrack) {
         auto result = std::make_shared<WaveformVZoomHandle>(
            pTrack, st.rect, st.state.m_y );
         result = AssignUIHandlePtr(mVZoomHandle, result);
         results.push_back(result);
      }
   }

   auto more = TrackVRulerControls::HitTest(st, pProject);
   std::copy(more.begin(), more.end(), std::back_inserter(results));

   return results;
}

unsigned WaveformVRulerControls::HandleWheelRotation(
   const TrackPanelMouseEvent &evt, AudacityProject *pProject)
{
   using namespace RefreshCode;
   const auto pTrack = FindTrack();
   if (!pTrack)
      return RefreshNone;
   const auto wt = static_cast<WaveTrack*>(pTrack.get());
   return DoHandleWheelRotation( evt, pProject, wt );
}

unsigned WaveformVRulerControls::DoHandleWheelRotation(
   const TrackPanelMouseEvent &evt, AudacityProject *pProject, WaveTrack *wt)
{
   using namespace RefreshCode;
   const wxMouseEvent &event = evt.event;

   if (!(event.ShiftDown() || event.CmdDown()))
      return RefreshNone;

   // Always stop propagation even if the ruler didn't change.  The ruler
   // is a narrow enough target.
   evt.event.Skip(false);

   auto steps = evt.steps;

   using namespace WaveTrackViewConstants;
   const bool isDB =
      wt->GetWaveformSettings().isLinear();
   // Special cases for Waveform dB only.
   // Set the bottom of the dB scale but only if it's visible
   if (isDB && event.ShiftDown() && event.CmdDown()) {
      float min, max;
      wt->GetDisplayBounds(&min, &max);
      if (!(min < 0.0 && max > 0.0))
         return RefreshNone;

      WaveformSettings &settings =
         wt->GetWaveformSettings();
      float olddBRange = settings.dBRange;
      for (auto channel : TrackList::Channels(wt)) {
         WaveformSettings &channelSettings =
            channel->GetWaveformSettings();
         if (steps < 0)
            // Zoom out
            channelSettings.NextLowerDBRange();
         else
            channelSettings.NextHigherDBRange();
      }

      float newdBRange = settings.dBRange;

      // Is y coordinate within the rectangle half-height centered about
      // the zero level?
      const auto &rect = evt.rect;
      const auto zeroLevel = wt->ZeroLevelYCoordinate(rect);
      const bool fixedMagnification =
      (4 * std::abs(event.GetY() - zeroLevel) < rect.GetHeight());

      if (fixedMagnification) {
         // Vary the db limit without changing
         // magnification; that is, peaks and troughs move up and down
         // rigidly, as parts of the wave near zero are exposed or hidden.
         const float extreme = (LINEAR_TO_DB(2) + newdBRange) / newdBRange;
         max = std::min(extreme, max * olddBRange / newdBRange);
         min = std::max(-extreme, min * olddBRange / newdBRange);
         for (auto channel : TrackList::Channels(wt)) {
            channel->SetLastdBRange();
            channel->SetDisplayBounds(min, max);
         }
      }
   }
   else if (event.CmdDown() && !event.ShiftDown()) {
      const int yy = event.m_y;
      WaveformVZoomHandle::DoZoom(
         pProject, wt,
         (steps < 0)
            ? kZoomOut
            : kZoomIn,
         evt.rect, yy, yy, true);
   }
   else if (!event.CmdDown() && event.ShiftDown()) {
      // Scroll some fixed number of pixels, independent of zoom level or track height:
      static const float movement = 10.0f;
      const int height = evt.rect.GetHeight();
      {
         float topLimit = 2.0;
         if (isDB) {
            const float dBRange = wt->GetWaveformSettings().dBRange;
            topLimit = (LINEAR_TO_DB(topLimit) + dBRange) / dBRange;
         }
         const float bottomLimit = -topLimit;
         float top, bottom;
         wt->GetDisplayBounds(&bottom, &top);
         const float range = top - bottom;
         const float delta = range * steps * movement / height;
         float newTop = std::min(topLimit, top + delta);
         const float newBottom = std::max(bottomLimit, newTop - range);
         newTop = std::min(topLimit, newBottom + range);
         for (auto channel : TrackList::Channels(wt))
            channel->SetDisplayBounds(newBottom, newTop);
      }
   }
   else
      return RefreshNone;

   ProjectHistory::Get( *pProject ).ModifyState(true);

   return RefreshCell | UpdateVRuler;
}

void WaveformVRulerControls::Draw(
   TrackPanelDrawingContext &context,
   const wxRect &rect_, unsigned iPass )
{
   TrackVRulerControls::Draw( context, rect_, iPass );
   WaveTrackVRulerControls::DoDraw( *this, context, rect_, iPass );
}

void WaveformVRulerControls::UpdateRuler( const wxRect &rect )
{
   const auto wt = std::static_pointer_cast< WaveTrack >( FindTrack() );
   if (!wt)
      return;
   DoUpdateVRuler( rect, wt.get() );
}

void WaveformVRulerControls::DoUpdateVRuler(
   const wxRect &rect, const WaveTrack *wt )
{
   auto vruler = &WaveTrackVRulerControls::ScratchRuler();

   // All waves have a ruler in the info panel
   // The ruler needs a bevelled surround.
   const float dBRange =
      wt->GetWaveformSettings().dBRange;

   float min, max;
   wt->GetDisplayBounds(&min, &max);

   if (dBRange != wt->GetLastdBRange())
   {
      wt->SetLastdBRange();
   }

   WaveformSettings::ScaleType scaleType =
   wt->GetWaveformSettings().scaleType;
   
   if (wt->GetWaveformSettings().isLinear()) {
      // Waveform

      if (wt->GetLastScaleType() != WaveformSettings::stLinearAmp &&
         wt->GetLastScaleType() != WaveformSettings::stLinearDb &&
          wt->GetLastScaleType() != -1)
      {
         // do a translation into the linear space
         wt->SetLastScaleType();
         wt->SetLastdBRange();
         float sign = (min >= 0 ? 1 : -1);
         if (min != 0.) {
            min = DB_TO_LINEAR(fabs(min) * dBRange - dBRange);
            if (min < 0.0)
               min = 0.0;
            min *= sign;
         }
         sign = (max >= 0 ? 1 : -1);
         
         if (max != 0.) {
            max = DB_TO_LINEAR(fabs(max) * dBRange - dBRange);
            if (max < 0.0)
               max = 0.0;
            max *= sign;
         }
         wt->SetDisplayBounds(min, max);
      }
      
      vruler->SetDbMirrorValue( 0.0 );
      vruler->SetBounds(rect.x, rect.y, rect.x + rect.width, rect.y + rect.height - 1);
      vruler->SetOrientation(wxVERTICAL);
      vruler->SetRange(max, min);
      vruler->SetFormat(std::make_unique<RealFormat>());
      RealFormatData formatData = { false };
      vruler->SetFormatData(formatData);
      if (scaleType == WaveformSettings::stLinearAmp) {
         vruler->SetLabelEdges(false);
         vruler->SetUnits({});
         vruler->SetUpdater(std::make_unique<LinearUpdater>());
      }
      else {
         RegenerateLinearDBValues(dBRange, min, max);
         vruler->SetLabelEdges(true);
         vruler->SetUnits(XO("dB"));
         vruler->SetUpdater(std::make_unique<CustomUpdaterValue>());
         RulerUpdater::Labels major, minor, minorMinor;
         std::vector<LinearDBValues> values = { majorValues, minorValues, minorMinorValues };
         for (int ii = 0; ii < 3; ii++) {
            RulerUpdater::Labels labs;
            int size = (ii == 0) ? majorValues.size() :
               (ii == 1) ? minorValues.size() : minorMinorValues.size();
            for (int i = 0; i < size; i++) {
               double value = (ii == 0) ? majorValues[i] :
                  (ii == 1) ? minorValues[i] : minorMinorValues[i];
               RulerUpdater::Label lab;

               if (value == -dBRange)
                  lab.value = 0;
               else {
                  float sign = (value > -dBRange ? 1 : -1);
                  if (value < -dBRange)
                     value = -2 * dBRange - value;
                  lab.value = DB_TO_LINEAR(value) * sign;
               }

               wxString s = (value == -dBRange) ?
                  wxString(L"-\u221e") : wxString::FromDouble(value);
               // \u221e represents the infinity symbol
               // Should this just be -dBRange so it is consistent?
               //wxString s = wxString::FromDouble(value);
               lab.text = Verbatim(s);

               labs.push_back(lab);
            }
            if (ii == 0)
               major = labs;
            else if (ii == 1)
               minor = labs;
            else
               minorMinor = labs;
         }
         CustomUpdaterData data = { major, minor, minorMinor };
         vruler->SetUpdaterData(data);
      }
   }
   else {
      vruler->SetUnits(XO("dB"));
      
      float lastdBRange;
      
      if (wt->GetLastScaleType() != WaveformSettings::stLogarithmicDb &&
         // When Logarithmic Amp happens, put that here
          wt->GetLastScaleType() != -1)
      {
         // do a translation into the dB space
         wt->SetLastScaleType();
         wt->SetLastdBRange();
         float sign = (min >= 0 ? 1 : -1);
         if (min != 0.) {
            min = (LINEAR_TO_DB(fabs(min)) + dBRange) / dBRange;
            if (min < 0.0)
               min = 0.0;
            min *= sign;
         }
         sign = (max >= 0 ? 1 : -1);
         
         if (max != 0.) {
            max = (LINEAR_TO_DB(fabs(max)) + dBRange) / dBRange;
            if (max < 0.0)
               max = 0.0;
            max *= sign;
         }
         wt->SetDisplayBounds(min, max);
      }
      else if (dBRange != (lastdBRange = wt->GetLastdBRange())) {
         wt->SetLastdBRange();
         // Remap the max of the scale
         float newMax = max;
         
         // This commented out code is problematic.
         // min and max may be correct, and this code cause them to change.
#ifdef ONLY_LABEL_POSITIVE
         const float sign = (max >= 0 ? 1 : -1);
         if (max != 0.) {
            
            // Ugh, duplicating from TrackPanel.cpp
#define ZOOMLIMIT 0.001f
            
            const float extreme = LINEAR_TO_DB(2);
            // recover dB value of max
            const float dB = std::min(extreme, (float(fabs(max)) * lastdBRange - lastdBRange));
            // find NEW scale position, but old max may get trimmed if the db limit rises
            // Don't trim it to zero though, but leave max and limit distinct
            newMax = sign * std::max(ZOOMLIMIT, (dBRange + dB) / dBRange);
            // Adjust the min of the scale if we can,
            // so the db Limit remains where it was on screen, but don't violate extremes
            if (min != 0.)
               min = std::max(-extreme, newMax * min / max);
         }
#endif
         wt->SetDisplayBounds(min, newMax);
      }
      
      // Old code was if ONLY_LABEL_POSITIVE were defined.
      // it uses the +1 to 0 range only.
      // the enabled code uses +1 to -1, and relies on set ticks labelling knowing about
      // the dB scale.
#ifdef ONLY_LABEL_POSITIVE
      if (max > 0) {
#endif
         int top = 0;
         float topval = 0;
         int bot = rect.height;
         float botval = -dBRange;
         
#ifdef ONLY_LABEL_POSITIVE
         if (min < 0) {
            bot = top + (int)((max / (max - min))*(bot - top));
            min = 0;
         }
         
         if (max > 1) {
            top += (int)((max - 1) / (max - min) * (bot - top));
            max = 1;
         }
         
         if (max < 1 && max > 0)
            topval = -((1 - max) * dBRange);
         
         if (min > 0) {
            botval = -((1 - min) * dBRange);
         }
#else
         topval = -((1 - max) * dBRange);
         botval = -((1 - min) * dBRange);
         vruler->SetDbMirrorValue( dBRange );
#endif
         vruler->SetBounds(rect.x, rect.y + top, rect.x + rect.width, rect.y + bot - 1);
         vruler->SetOrientation(wxVERTICAL);
         vruler->SetRange(topval, botval);
#ifdef ONLY_LABEL_POSITIVE
      }
      else
         vruler->SetBounds(0.0, 0.0, 0.0, 0.0); // A.C.H I couldn't find a way to just disable it?
#endif
      vruler->SetFormat(std::make_unique<RealFormat>());
      RealFormatData formatData = { true };
      vruler->SetFormatData(formatData);
      vruler->SetLabelEdges(true);
      vruler->SetUpdater(std::make_unique<LinearUpdater>());
   }
   vruler->GetMaxSize( &wt->vrulerSize.first, &wt->vrulerSize.second );
}
