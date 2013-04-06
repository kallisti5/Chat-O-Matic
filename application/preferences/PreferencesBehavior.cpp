/*
 * Copyright 2010, Oliver Ruiz Dorantes. All rights reserved.
 * Copyright 2012, Dario Casalinuovo. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <Button.h>
#include <CheckBox.h>
#include <ControlLook.h>
#include <Deskbar.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <ScrollView.h>
#include <StringView.h>

#include "AccountManager.h"
#include "CayaProtocol.h"
#include "PreferencesBehavior.h"
#include "CayaPreferences.h"
#include "ProtocolManager.h"
#include "ProtocolSettings.h"
#include "MainWindow.h"
#include "ReplicantStatusView.h"
#include "TheApp.h"

const uint32 kToCurrentWorkspace = 'CBcw';
const uint32 kRaiseOnMessageReceived = 'FCmr';
const uint32 kRaiseUserIsTyping = 'FCit';
const uint32 kNotifyProtocolsLogin = 'NTpl';
const uint32 kNotifyContactStatus = 'NTcl';
const uint32 kNotifyNewMessage = 'NTms';
const uint32 kMarkUnreadWindow = 'MKuw';
const uint32 kHideOffline = 'HiOf';


PreferencesBehavior::PreferencesBehavior()
	: BView("Behavior", B_WILL_DRAW) 
{

	fOnIncoming = new BStringView("onIncoming", "On incoming message...");
	fOnIncoming->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT, B_ALIGN_MIDDLE));
	fOnIncoming->SetFont(be_bold_font);
	
	fHideOffline = new BCheckBox("HideOfflineContacts",
		"Hide offline contacts",
		new BMessage(kHideOffline));

	fToCurrentWorkspace = new BCheckBox("ToCurrentWorkspace",
		"Move window to current workspace",
		new BMessage(kToCurrentWorkspace));

	fRaiseOnMessageReceived = new BCheckBox("FocusOnMessageReceived",
		"Auto-raise when a message is received",
		new BMessage(kRaiseOnMessageReceived));

	fRaiseUserIsTyping = new BCheckBox("FocusUserIsTyping",
		"Auto-raise when user is typing",
		new BMessage(kRaiseUserIsTyping));

	fPlaySoundOnMessageReceived = new BCheckBox("PlaySoundOnMessageReceived",
		"Play sound event", NULL);
	fPlaySoundOnMessageReceived->SetEnabled(false);  // not implemented

	fMarkUnreadWindow = new BCheckBox("MarkUnreadWindow",
		"Mark unread window chat", new BMessage(kMarkUnreadWindow));
	/*fMarkUnreadWindow->SetEnabled(false); implementing it right now*/
	
	fMarkUnreadReplicant = new BCheckBox("MarkUnreadReplicant",
		"Mark unread the Deskbar Replicant", NULL);
	fMarkUnreadReplicant->SetEnabled(false);
			// not implemented

	fNotifications = new BStringView("notifications",
						"Deskbar Notifications (experimental)");

	fNotifications->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT, B_ALIGN_MIDDLE));
	fNotifications->SetFont(be_bold_font);

	fNotifyProtocols = new BCheckBox("EnableProtocolNotify",
		"Enable protocol status notifications",new BMessage(kNotifyProtocolsLogin));

	fNotifyContactStatus = new BCheckBox("EnableContactNotify",
		"Enable contact status notifications",new BMessage(kNotifyContactStatus));
	
	fNotifyNewMessage = new BCheckBox("EnableMessageNotify",
		"Enable message notifications", new BMessage(kNotifyNewMessage));

	const float spacing = be_control_look->DefaultItemSpacing();

	SetLayout(new BGroupLayout(B_HORIZONTAL, spacing));
	AddChild(BGroupLayoutBuilder(B_VERTICAL)
		.Add(fOnIncoming)
		.AddGroup(B_VERTICAL, spacing)
			.Add(fHideOffline)
			.Add(fToCurrentWorkspace)
			.Add(fRaiseOnMessageReceived)
			.Add(fRaiseUserIsTyping)
			.Add(fMarkUnreadWindow)
			.Add(fMarkUnreadReplicant)
			.Add(fPlaySoundOnMessageReceived)
		.	SetInsets(spacing * 2, spacing, spacing, spacing)
		.End()
		.Add(fNotifications)
		.AddGroup(B_VERTICAL, spacing)
			.Add(fNotifyProtocols)
			.Add(fNotifyContactStatus)
			.Add(fNotifyNewMessage)
		.	SetInsets(spacing * 2, spacing, spacing, spacing)
		.End()
		.AddGlue()
		.SetInsets(spacing, spacing, spacing, spacing)
		.TopView()
	);
}


void
PreferencesBehavior::AttachedToWindow()
{
	fHideOffline->SetTarget(this);
	fToCurrentWorkspace->SetTarget(this);
	fRaiseUserIsTyping->SetTarget(this);
	fRaiseOnMessageReceived->SetTarget(this);
	fNotifyProtocols->SetTarget(this);
	fNotifyContactStatus->SetTarget(this);
	fNotifyNewMessage->SetTarget(this);
	
	fHideOffline->SetValue(
		CayaPreferences::Item()->HideOffline);
	fToCurrentWorkspace->SetValue(
		CayaPreferences::Item()->MoveToCurrentWorkspace);
	fRaiseUserIsTyping->SetValue(
		CayaPreferences::Item()->RaiseUserIsTyping);
	fRaiseOnMessageReceived->SetValue(
		CayaPreferences::Item()->RaiseOnMessageReceived);
	fMarkUnreadWindow->SetValue(
		CayaPreferences::Item()->MarkUnreadWindow);
	fNotifyProtocols->SetValue(
		CayaPreferences::Item()->NotifyProtocolStatus);
	fNotifyContactStatus->SetValue(
		CayaPreferences::Item()->NotifyContactStatus);
	fNotifyNewMessage->SetValue(
		CayaPreferences::Item()->NotifyNewMessage);
}


void
PreferencesBehavior::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kHideOffline:
			CayaPreferences::Item()->HideOffline
				= fHideOffline->Value();
			break;
		case kToCurrentWorkspace:
			CayaPreferences::Item()->MoveToCurrentWorkspace
				= fToCurrentWorkspace->Value();
			break;
		case kRaiseOnMessageReceived:
			CayaPreferences::Item()->RaiseOnMessageReceived
				= fRaiseOnMessageReceived->Value();
			break;
		case kRaiseUserIsTyping:
			CayaPreferences::Item()->RaiseUserIsTyping
				= fRaiseUserIsTyping->Value();
			break;
		case kNotifyProtocolsLogin:
			CayaPreferences::Item()->NotifyProtocolStatus
				= fNotifyProtocols->Value();
			break;
		case kNotifyContactStatus:
			CayaPreferences::Item()->NotifyContactStatus
				= fNotifyContactStatus->Value();
			break;
		case kNotifyNewMessage:
			CayaPreferences::Item()->NotifyNewMessage
				= fNotifyNewMessage->Value();
			break;
		case kMarkUnreadWindow:
			CayaPreferences::Item()->MarkUnreadWindow
				= fMarkUnreadWindow->Value();
			break;
		default:
			BView::MessageReceived(message);
	}
}
