//****************************************************************************************
//
//	File:		PulseView.h
//
//	Written by:	David Ramsey and Daniel Switkin
//
//	Copyright 1999, Be Incorporated
//
//	Revised by: Steffen Yount
//
//****************************************************************************************

#ifndef PULSEVIEW_H
#define PULSEVIEW_H

#include <View.h>
#include <PopUpMenu.h>
#include <MenuItem.h>
#include "Common.h"
#include "PulseApp.h"
#include "Prefs.h"



class PulseView : public BView {
	public:
		PulseView(BRect rect, const char *name);
		PulseView(BMessage *message);
		~PulseView();
		virtual void MouseDown(BPoint point);
		void ChangeCPUState(BMessage *message);

	protected:
		void Init();
		void Update();

		BPopUpMenu *popupmenu;
		BMenuItem *mode1, *mode2, *preferences, *about;
		BMenuItem **cpu_menu_items;
		
		double cpu_times[B_MAX_CPU_COUNT];
		bigtime_t prev_active[B_MAX_CPU_COUNT];
		bigtime_t prev_time;
};

#endif

