/*
Gwenview - A simple image viewer for KDE
Copyright (C) 2000-2002 Aur�lien G�teau
 
This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.
 
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
 
You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 
*/

#ifndef GVSLIDESHOW_H
#define GVSLIDESHOW_H

#include <qobject.h>

class KAction;
class QTimer;

class GVSlideShow : public QObject
{
Q_OBJECT
public:
	GVSlideShow(KAction* first,KAction* next);
	
	void setLoop(bool);
	bool loop() const { return mLoop; }
	
	void setDelay(int);
	int delay() const { return mDelay; }
	
	void start();
	void stop();

	void readConfig(KConfig* config,const QString& group);
	void writeConfig(KConfig* config,const QString& group) const;

signals:
	void finished();

private slots:
	void slotTimeout();

private:
	QTimer* mTimer;
	KAction* mFirst;
	KAction* mNext;
	int mDelay;
	bool mLoop;
};

#endif // GVSLIDESHOW_H

