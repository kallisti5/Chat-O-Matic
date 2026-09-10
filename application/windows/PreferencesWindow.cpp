/*
 * Copyright 2009-2010, Pier Luigi Fiorini. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Pier Luigi Fiorini, pierluigi.fiorini@gmail.com
 */

#include "PreferencesWindow.h"

#include <Button.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <LayoutBuilder.h>
#include <TabView.h>

#include "PreferencesBehavior.h"
#include "PreferencesChatWindow.h"
#include "PreferencesNotifications.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PreferencesWindow"


const uint32 kApply = 'SAVE';


PreferencesWindow::PreferencesWindow()
	: BWindow(BRect(0, 0, 500, 615), B_TRANSLATE("Preferences"),
		B_TITLED_WINDOW, B_AUTO_UPDATE_SIZE_LIMITS | B_CLOSE_ON_ESCAPE)
{
	BTabView* tabView = new BTabView("tabView", B_WIDTH_AS_USUAL);
	tabView->AddTab(new PreferencesBehavior());
	tabView->AddTab(new PreferencesChatWindow());
	tabView->AddTab(new PreferencesNotifications());

	float charCount = 0;
	for (int i = 0; i < tabView->CountTabs(); i++)
		charCount += strlen(tabView->TabAt(i)->Label());

	float fontScale = be_plain_font->Size();
	// Smooth linear formula: smaller fonts need more padding per character,
	// larger fonts need less. Interpolated from the original switch values.
	float padding = 17.0f - (fontScale - 8.0f) * 0.85f;
	if (padding < 0.0f)
		padding = 0.0f;
	float minWidth = (charCount + padding) * fontScale;
	if (minWidth < 300.0f)
		minWidth = 300.0f;
	tabView->SetExplicitMinSize(BSize(minWidth, B_SIZE_UNSET));

	BButton* ok = new BButton(B_TRANSLATE("OK"), new BMessage(kApply));

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.Add(tabView)
		.AddGroup(B_HORIZONTAL)
		.SetInsets(0, 0, B_USE_HALF_ITEM_SPACING, B_USE_HALF_ITEM_SPACING)
			.AddGlue()
			.Add(ok)
		.End()
	.End();

	CenterOnScreen();
}


void
PreferencesWindow::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case kApply:
			Close();
			break;
		default:
			BWindow::MessageReceived(msg);
	}
}


