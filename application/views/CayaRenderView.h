#ifndef _CayaRenderView_H
#define _CayaRenderView_H_

#include <librunview/RunView.h>

#include <librunview/SmileTextRender.h>

class RunView;
class Theme;


enum CayaRenderViewColors {
	COL_URL =	 0,
	COL_TIMESTAMP,
	COL_TEXT,
	COL_OWNNICK,
	COL_OTHERNICK,
	COL_ACTION,
	COL_SELECTION,
	COL_TIMESTAMP_DUMMY,
	COL_MAX_COLORS
};

enum {
	R_URL = 0,
	R_TEXT,	
	R_TIMESTAMP,
	R_ACTION,
	R_EMOTICON,
	R_TIMESTAMP_DUMMY,
	MAX_RENDERS
};


class CayaRenderView : public RunView 
{
	public:
				CayaRenderView(const char* name, const char* smileyConfig = NULL);
		
		void	AppendMessage(const char* nick, const char* message, rgb_color nameColor);
		void	AppendGenericMessage(const char* message);
		void 	AddEmoticText(const char * txt, rgb_color fore, rgb_color bg);
		
	protected:
		void	PrepareTheme(Theme* theme);
	
	private:
		Theme* fTheme;
		SmileTextRender	str;
	
};

#endif
