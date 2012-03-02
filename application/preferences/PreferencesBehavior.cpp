/*
 * Copyright 2010, Oliver Ruiz Dorantes. All rights reserved.
 * Copyright 2012, Dario Casalinuovo. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <Button.h>
#include <CheckBox.h>
#include <ControlLook.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <ScrollView.h>
#include <StringView.h>

#include "CayaProtocol.h"
#include "PreferencesBehavior.h"
#include "CayaPreferences.h"
#include "ProtocolManager.h"
#include "ProtocolSettings.h"
#include "MainWindow.h"
#include "TheApp.h"

const uint32 kToCurrentWorkspace = 'CBcw';
const uint32 kActivateChatWindow = 'CBac';
const uint32 kDisableReplicant = 'DSrp';
const uint32 kPermanentReplicant ='PRpt';
const uint32 kHideCayaTracker = 'HCtk';



PreferencesBehavior::PreferencesBehavior()
	: BView("Behavior", B_WILL_DRAW)
{

	fOnIncoming = new BStringView("onIncoming", "On incoming message...");
	fOnIncoming->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT, B_ALIGN_MIDDLE));
	fOnIncoming->SetFont(be_bold_font);

	fToCurrentWorkspace = new BCheckBox("ToCurrentWorkspace",
		"Move window to current workspace",
		new BMessage(kToCurrentWorkspace));

	fActivateChatWindow = new BCheckBox("ActivateChatWindow",
		"Get focus ",
		new BMessage(kActivateChatWindow));

	fPlaySoundOnMessageReceived = new BCheckBox("PlaySoundOnMessageReceived",
		"Play sound event", NULL);
	fPlaySoundOnMessageReceived->SetEnabled(false);  // not implemented

	fMarkUnreadWindow = new BCheckBox("MarkUnreadWindow",
		"Mark unread window chat", NULL);
	fMarkUnreadWindow->SetEnabled(false);
			// not implemented

	fReplicantString = new BStringView("ReplicantString", "Replicant");
	fReplicantString->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT, B_ALIGN_MIDDLE));
	fReplicantString->SetFont(be_bold_font);

	fDisableReplicant = new BCheckBox("DisableReplicant",
		"Disable Deskbar replicant", NULL);
	fDisableReplicant->SetEnabled(true);

	fPermanentReplicant = new BCheckBox("PermanentReplicant",
		"Permanent Deskbar Replicant", NULL);
	fPermanentReplicant->SetEnabled(false);

	fHideCayaDeskbar = new BCheckBox("HideCayaDeskbar",
		"Hide Caya field in Deskbar", NULL);
	fHideCayaDeskbar->SetEnabled(true);

	const float spacing = be_control_look->DefaultItemSpacing();

	SetLayout(new BGroupLayout(B_HORIZONTAL, spacing));
	AddChild(BGroupLayoutBuilder(B_VERTICAL)
		.Add(fOnIncoming)
		.AddGroup(B_VERTICAL, spacing)
			.Add(fToCurrentWorkspace)
			.Add(fActivateChatWindow)
			.Add(fMarkUnreadWindow)
			.Add(fPlaySoundOnMessageReceived)
		.SetInsets(spacing * 2, spacing, spacing, spacing)
		.End()
		.Add(fReplicantString)
		.AddGroup(B_VERTICAL, spacing)
			.Add(fDisableReplicant)
			.Add(fPermanentReplicant)
			.Add(fHideCayaDeskbar)
			.SetInsets(spacing * 2, spacing, spacing, spacing)
		.End()
		.AddGlue()
		.SetInsets(spacing, spacing, spacing, spacing)
		.TopView()
	);
}


void
PreferencesBehavior::AttachedToWindow()
{
	fToCurrentWorkspace->SetTarget(this);
	fActivateChatWindow->SetTarget(this);

	fToCurrentWorkspace->SetValue(
		CayaPreferences::Item()->MoveToCurrentWorkspace);
	fActivateChatWindow->SetValue(
		CayaPreferences::Item()->ActivateWindow);
}


void
PreferencesBehavior::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kToCurrentWorkspace:
			CayaPreferences::Item()->MoveToCurrentWorkspace
				= fToCurrentWorkspace->Value();
			break;
		case kActivateChatWindow:
			CayaPreferences::Item()->ActivateWindow
				= fActivateChatWindow->Value();
			break;
		default:
			BView::MessageReceived(message);
	}
}
