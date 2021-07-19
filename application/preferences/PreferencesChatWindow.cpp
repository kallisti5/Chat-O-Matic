/*
 * Copyright 2010, Oliver Ruiz Dorantes. All rights reserved.
 * Copyright 2012, Dario Casalinuovo. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <Catalog.h>
#include <CheckBox.h>
#include <ControlLook.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <StringView.h>

#include "ChatProtocol.h"
#include "PreferencesChatWindow.h"
#include "AppPreferences.h"
#include "ProtocolManager.h"
#include "ProtocolSettings.h"
#include "MainWindow.h"
#include "TheApp.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PreferencesChatWindow"


const uint32 kIgnoreEmoticons = 'CBhe';


PreferencesChatWindow::PreferencesChatWindow()
	: BView(B_TRANSLATE("Chat display"), B_WILL_DRAW)
{

	fChatWindowString = new BStringView("ChatWindowString",
		B_TRANSLATE("Chat settings"));
	fChatWindowString->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT, B_ALIGN_MIDDLE));
	fChatWindowString->SetFont(be_bold_font);

	fIgnoreEmoticons = new BCheckBox("IgnoreEmoticons",
		B_TRANSLATE("Ignore emoticons"),
		new BMessage(kIgnoreEmoticons));
	fIgnoreEmoticons->SetEnabled(true);

	const float spacing = be_control_look->DefaultItemSpacing();

	SetLayout(new BGroupLayout(B_HORIZONTAL, spacing));
	AddChild(BGroupLayoutBuilder(B_VERTICAL)
		.Add(fChatWindowString)
		.AddGroup(B_VERTICAL, spacing)
			.Add(fIgnoreEmoticons)
			.SetInsets(spacing * 2, spacing, spacing, spacing)
		.End()
		.AddGlue()
		.SetInsets(spacing, spacing, spacing, spacing)
		.TopView()
	);
}


void
PreferencesChatWindow::AttachedToWindow()
{
	fIgnoreEmoticons->SetTarget(this);
	fIgnoreEmoticons->SetValue(
		AppPreferences::Item()->IgnoreEmoticons);

}


void
PreferencesChatWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kIgnoreEmoticons:
			AppPreferences::Item()->IgnoreEmoticons
				= fIgnoreEmoticons->Value();
			break;
		default:
			BView::MessageReceived(message);
	}
}
