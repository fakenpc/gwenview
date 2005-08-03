// vim: set tabstop=4 shiftwidth=4 noexpandtab
/*
Gwenview - A simple image viewer for KDE
Copyright 2000-2004 Aur�lien G�teau

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

// STL
#include <algorithm>

// Qt
#include <qtimer.h>

// KDE
#include <kconfig.h>
#include <kdebug.h>

// Local
#include "slideshow.moc"

#include "document.h"
#include "gvconfig.h"
#include "imageloader.h"

namespace Gwenview {


static const char* CONFIG_DELAY="delay";
static const char* CONFIG_LOOP="loop";


SlideShow::SlideShow(Document* document)
: mDelay(10), mLoop(false), mDocument(document), mStarted(false), mPrefetch( NULL ) {
	mTimer=new QTimer(this);
	connect(mTimer, SIGNAL(timeout()),
			this, SLOT(slotTimeout()) );
	connect(mDocument, SIGNAL(loaded(const KURL&)),
			this, SLOT(slotLoaded()) );
}

SlideShow::~SlideShow() {
}

void SlideShow::setLoop(bool value) {
	mLoop=value;
}

void SlideShow::setDelay(int delay) {
	mDelay=delay;
	if (mTimer->isActive()) {
		mTimer->changeInterval(delay*1000);
	}
}

void SlideShow::setRandom(bool value) {
	mRandom=value;
}

void SlideShow::start(const KURL::List& urls) {
	mURLs.resize(urls.size());
	qCopy(urls.begin(), urls.end(), mURLs.begin());
	if (mRandom) {
		std::random_shuffle(mURLs.begin(), mURLs.end());
	}

	mStartIt=qFind(mURLs.begin(), mURLs.end(), mDocument->url());
	if (mStartIt==mURLs.end()) {
		kdWarning() << k_funcinfo << "Current URL not found in list, aborting.\n";
		return;
	}
	
	mTimer->start(mDelay*1000, true);
	mStarted=true;
	prefetch();
}


void SlideShow::stop() {
	mTimer->stop();
	mStarted=false;
}


void SlideShow::slotTimeout() {
	QValueVector<KURL>::ConstIterator it=qFind(mURLs.begin(), mURLs.end(), mDocument->url());
	if (it==mURLs.end()) {
		kdWarning() << k_funcinfo << "Current URL not found in list, aborting.\n";
		stop();
		emit finished();
		return;
	}

	++it;
	if (it==mURLs.end()) {
		it=mURLs.begin();
	}

	if (it==mStartIt && !mLoop) {
		stop();
		emit finished();
		return;
	}

	emit nextURL(*it);
}


void SlideShow::slotLoaded() {
	if (mStarted) {
		mTimer->start(mDelay*1000, true);
		prefetch();
	}
}


void SlideShow::prefetch() {
	QValueVector<KURL>::ConstIterator it=qFind(mURLs.begin(), mURLs.end(), mDocument->url());
	if (it==mURLs.end()) {
		return;
	}

	++it;
	if (it==mURLs.end()) {
		it=mURLs.begin();
	}

	if (it==mStartIt && !mLoop) {
		return;
	}

	if( mPrefetch != NULL ) mPrefetch->release( this );
	mPrefetch = ImageLoader::loader( *it, this, BUSY_PRELOADING );
	connect( mPrefetch, SIGNAL( imageLoaded( bool )), SLOT( prefetchDone()));
}

void SlideShow::prefetchDone() {
	if( mPrefetch != NULL ) { 
		mPrefetch->release( this );
		mPrefetch = NULL;
	}
}

//-Configuration--------------------------------------------
void SlideShow::readConfig(KConfig* config,const QString& group) {
	config->setGroup(group);
	mDelay=config->readNumEntry(CONFIG_DELAY,10);
	mLoop=config->readBoolEntry(CONFIG_LOOP,false);
	mRandom=GVConfig::self()->slideShowRandom();
}


void SlideShow::writeConfig(KConfig* config,const QString& group) const {
	config->setGroup(group);
	config->writeEntry(CONFIG_DELAY,mDelay);
	config->writeEntry(CONFIG_LOOP,mLoop);
	GVConfig::self()->setSlideShowRandom(mRandom);
}

} // namespace
