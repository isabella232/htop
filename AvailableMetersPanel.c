/*
htop - AvailableMetersPanel.c
(C) 2004-2011 Hisham H. Muhammad
Released under the GNU GPL, see the COPYING file
in the source distribution for its full text.
*/

#include "AvailableMetersPanel.h"
#include "MetersPanel.h"

#include "CPUMeter.h"
#include "DDMeter.h"
#include "Header.h"
#include "ListItem.h"
#include "Platform.h"

#include <assert.h>
#include <stdlib.h>

/*{
#include "Settings.h"
#include "Panel.h"
#include "ScreenManager.h"
#include "ProcessList.h"

typedef struct AvailableMetersPanel_ {
   Panel super;
   ScreenManager* scr;

   Settings* settings;
   Header* header;
   Panel* leftPanel;
   Panel* rightPanel;
} AvailableMetersPanel;

}*/

static void AvailableMetersPanel_delete(Object* object) {
   Panel* super = (Panel*) object;
   AvailableMetersPanel* this = (AvailableMetersPanel*) object;
   Panel_done(super);
   free(this);
}

static inline void AvailableMetersPanel_addMeter(Header* header, Panel* panel, MeterClass* type, int param, int column) {
   Meter* meter = (Meter*) Header_addMeterByClass(header, type, param, column);
   Panel_add(panel, (Object*) Meter_toListItem(meter, false));
   Panel_setSelected(panel, Panel_size(panel) - 1);
   MetersPanel_setMoving((MetersPanel*)panel, true);
   FunctionBar_draw(panel->currentBar, NULL);
}

static HandlerResult AvailableMetersPanel_eventHandler(Panel* super, int ch) {
   AvailableMetersPanel* this = (AvailableMetersPanel*) super;
   Header* header = this->header;

   ListItem* selected = (ListItem*) Panel_getSelected(super);
   int param = selected->key & 0xff;
   int type = selected->key >> 16;
   HandlerResult result = IGNORED;
   bool update = false;

   switch(ch) {
      case KEY_F(5):
      case 'l':
      case 'L':
      {
         AvailableMetersPanel_addMeter(header, this->leftPanel, Platform_meterTypes[type], param, 0);
         result = HANDLED;
         update = true;
         break;
      }
      case 0x0a:
      case 0x0d:
      case KEY_ENTER:
      case KEY_F(6):
      case 'r':
      case 'R':
      {
         AvailableMetersPanel_addMeter(header, this->rightPanel, Platform_meterTypes[type], param, 1);
         result = (KEY_LEFT << 16) | SYNTH_KEY;
         update = true;
         break;
      }
   }
   if (update) {
      this->settings->changed = true;
      Header_calculateHeight(header);
      Header_draw(header);
      ScreenManager_resize(this->scr, this->scr->x1, header->height, this->scr->x2, this->scr->y2);
   }
   return result;
}

PanelClass AvailableMetersPanel_class = {
   .super = {
      .extends = Class(Panel),
      .delete = AvailableMetersPanel_delete
   },
   .eventHandler = AvailableMetersPanel_eventHandler
};

AvailableMetersPanel* AvailableMetersPanel_new(Settings* settings, Header* header, Panel* leftMeters, Panel* rightMeters, ScreenManager* scr, ProcessList* pl) {
   AvailableMetersPanel* this = AllocThis(AvailableMetersPanel);
   Panel* super = (Panel*) this;
   FunctionBar* fuBar = FunctionBar_newEnterEsc("Add   ", "Done   ");
   Panel_init(super, 1, 1, 1, 1, Class(ListItem), true, fuBar);

   this->settings = settings;
   this->header = header;
   this->leftPanel = leftMeters;
   this->rightPanel = rightMeters;
   this->scr = scr;

   Panel_setHeader(super, "Available Meters");
   for (int i = 1; Platform_meterTypes[i]; i++) {
      MeterClass* type = Platform_meterTypes[i];
      // Do not dispay the meters for the CPU and the Datadog metrics, they zill be treated later
      if (type != &CPUMeter_class && type != &DDMeter_class) {
         const char* label = type->description ? type->description : type->uiName;
         Panel_add(super, (Object*) ListItem_new(label, i << 16));
      }
   }

   MeterClass* type = &DDMeter_class;
   for (int i = 0; i < METRICS_NUMBER; i++) {
      char buffer[50];
      sprintf(buffer, "%s %s", type->uiName, ddmetrics[i]);
      Panel_add(super, (Object*) ListItem_new(buffer, (1 << 16) + i));
   }

   type = &CPUMeter_class;
   int cpus = pl->cpuCount;
   if (cpus > 1) {
      Panel_add(super, (Object*) ListItem_new("CPU average", 0));
      for (int i = 1; i <= cpus; i++) {
         char buffer[50];
         sprintf(buffer, "%s %d", type->uiName, i);
         Panel_add(super, (Object*) ListItem_new(buffer, i));
      }
   } else {
      Panel_add(super, (Object*) ListItem_new("CPU", 1));
   }
   return this;
}
