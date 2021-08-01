/*
 * Copyright 2021, Jaidyn Levesque <jadedctrl@teknik.io>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "SendTextView.h"

#include <StringList.h>
#include <Window.h>

#include "AppMessages.h"
#include "MainWindow.h"
#include "Server.h"
#include "TheApp.h"


SendTextView::SendTextView(const char* name, ConversationView* convView)
	:
	BTextView(name),
	fConversationView(convView),
	fCurrentIndex(0),
	fHistoryIndex(0)
{
}


void
SendTextView::KeyDown(const char* bytes, int32 numBytes)
{
	int32 modifiers = Window()->CurrentMessage()->GetInt32("modifiers", 0);

	if (bytes[0] == B_TAB) {
		_AutoComplete();
		return;
	}
	// Reset auto-complete state if user typed/sent something other than tab
	fCurrentIndex = 0;
	fCurrentWord.SetTo("");

	if ((bytes[0] == B_UP_ARROW) && (modifiers == 0)) {
		_UpHistory();
		return;
	}
	else if ((bytes[0] == B_DOWN_ARROW) && (modifiers == 0)) {
		_DownHistory();
		return;
	}

	if ((bytes[0] == B_ENTER) && (modifiers & B_COMMAND_KEY))
		Insert("\n");
	else if ((bytes[0] == B_ENTER) && (modifiers == 0)) {
		_AppendHistory();
		fConversationView->MessageReceived(new BMessage(APP_CHAT));
	}
	else
		BTextView::KeyDown(bytes, numBytes);
}


void
SendTextView::_AutoComplete()
{
	if (fConversationView == NULL || fConversationView->GetConversation() == NULL)
		return;

	BStringList words;
	BString text = Text();
	text.Split(" ", true, words);
	if (words.CountStrings() <= 0)
		return;
	BString lastWord = words.StringAt(words.CountStrings() - 1);

	if (fCurrentWord.IsEmpty() == true)
		fCurrentWord = lastWord;

	// Now to find the substitutes
	const char* substitution = NULL;
	if (fCurrentWord.StartsWith("/") == true)
		substitution = _CommandAutoComplete();
	else
		substitution = _UserAutoComplete();

	if (substitution == NULL)
		fCurrentIndex = 0;
	else {
		text.ReplaceLast(lastWord.String(), substitution);
		SetText(text.String());
		Select(TextLength(), TextLength());
	}
}


const char*
SendTextView::_CommandAutoComplete()
{
	int64 instance =
		fConversationView->GetConversation()->GetProtocolLooper()->GetInstance();
	CommandMap commands =
		((TheApp*)be_app)->GetMainWindow()->GetServer()->Commands(instance);

	BString command;
	BString cmdWord = BString(fCurrentWord).Remove(0, 1);
	for (int i, j = 0; i < commands.CountItems(); i++)
		if (commands.KeyAt(i).StartsWith(cmdWord)) {
			if (j == fCurrentIndex) {
				command = commands.KeyAt(i);
				fCurrentIndex++;
				break;
			}
			j++;
		}
	if (command.IsEmpty() == true)
		return NULL;
	return command.Prepend("/").String();
}


const char*
SendTextView::_UserAutoComplete()
{
	BString user;
	UserMap users = fConversationView->GetConversation()->Users();
	for (int i, j = 0; i < users.CountItems(); i++)
		if (users.KeyAt(i).StartsWith(fCurrentWord)) {
			if (j == fCurrentIndex) {
				user = users.KeyAt(i);
				fCurrentIndex++;
				break;
			}
			j++;
		}
	if (user.IsEmpty() == true)
		return NULL;
	return user.String();
}


void
SendTextView::_AppendHistory()
{
	fHistoryIndex = 0;
	fHistory.Add(BString(Text()), 0);
	if (fHistory.CountStrings() == 21)
		fHistory.Remove(20);
}


void
SendTextView::_UpHistory()
{
	if (fHistoryIndex == 0 && TextLength() > 0)
		_AppendHistory();

	if (fHistoryIndex < fHistory.CountStrings()) {
		fHistoryIndex++;
		SetText(fHistory.StringAt(fHistoryIndex - 1));
		Select(TextLength(), TextLength());
	}
}


void
SendTextView::_DownHistory()
{
	if (fHistoryIndex > 1) {
		fHistoryIndex--;
		SetText(fHistory.StringAt(fHistoryIndex - 1));
		Select(TextLength(), TextLength());
	}
}
