// vim: set tabstop=4 shiftwidth=4 noexpandtab
// kate: indent-mode csands; indent-width 4; replace-tabs-save off; replace-tabs off; replace-trailing-space-save off; space-indent off; tabs-indents on; tab-width 4;
/*
Gwenview - A simple image viewer for KDE
Copyright 2000-2004 Aur�ien G�eau

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
#include "cache.h"

namespace Gwenview {


static const char* CONFIG_START_FULLSCREEN="fullscreen";
static const char* CONFIG_STOP_AT_END="stopAtEnd";
static const char* CONFIG_RANDOM="random";
static const char* CONFIG_DELAY_SUFFIX="delaySuffix";
static const char* CONFIG_DELAY="delay";
static const char* CONFIG_LOOP="loop";


SlideShow::SlideShow(Document* document)
: mStopAtEnd(true), mDelay(10), mLoop(false), mDocument(document), mStarted(false), mPrefetch( NULL ) {
	mTimer=new QTimer(this);
	connect(mTimer, SIGNAL(timeout()),
			this, SLOT(slotTimeout()) );
	connect(mDocument, SIGNAL(loaded(const KURL&)),
			this, SLOT(slotLoaded()) );
}

SlideShow::~SlideShow() {
	if( !mPriorityURL.isEmpty()) Cache::instance()->setPriorityURL( mPriorityURL, false );
}


void SlideShow::setLoop(bool value) {
	mLoop=value;
}


void SlideShow::setDelay(int value) {
	mDelay=value;
    
	if (mTimer->isActive()) {
		mTimer->changeInterval(delayTimer());
	}
}


int SlideShow::delayTimer() const {
	if (mDelaySuffix == "s") {
		return mDelay*1000;
	} else {
		return mDelay;
	}
}


void SlideShow::setDelaySuffix(QString suffix) {
	mDelaySuffix=suffix;
}


void SlideShow::setFullscreen(bool fullscreen) {
	mFullscreen=fullscreen;
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
	
	mTimer->start(delayTimer(), true);
	mStarted=true;
	prefetch();
	emit stateChanged(true);
}


void SlideShow::stop() {
	mTimer->stop();
	mStarted=false;
	emit stateChanged(false);
	if( !mPriorityURL.isEmpty()) {
		Cache::instance()->setPriorityURL( mPriorityURL, false );
		mPriorityURL = KURL();
	}
}


QValueVector<KURL>::ConstIterator SlideShow::findNextURL() const {
	QValueVector<KURL>::ConstIterator it=qFind(mURLs.begin(), mURLs.end(), mDocument->url());
	if (it==mURLs.end()) {
		kdWarning() << k_funcinfo << "Current URL not found in list. This should not happen.\n";
		return it;
	}

	++it;
	if (it==mURLs.end()) {
		if (mStopAtEnd) {
			return it;
		} else {
			it=mURLs.begin();
		}
	}

	if (it==mStartIt && !mLoop) {
		return mURLs.end();
	}

	return it;
}


void SlideShow::slotTimeout() {
	// wait for prefetching to finish
	if( mPrefetch != NULL ) {
		return;
	}

	QValueVector<KURL>::ConstIterator it=findNextURL();
	if (it==mURLs.end()) {
		stop();
		return;
	}
	emit nextURL(*it);
}


void SlideShow::slotLoaded() {
	if (mStarted) {
		mTimer->start(delayTimer(), true);
		prefetch();
	}
}


void SlideShow::prefetch() {
	QValueVector<KURL>::ConstIterator it=findNextURL();
	if (it==mURLs.end()) {
		return;
	}

	if( mPrefetch != NULL ) mPrefetch->release( this );
	// TODO don't use prefetching with disabled optimizations (and add that option ;) )
	// (and also don't use prefetching in other places if the image won't fit in cache)
	mPrefetch = ImageLoader::loader( *it, this, BUSY_PRELOADING );
	if( !mPriorityURL.isEmpty()) Cache::instance()->setPriorityURL( mPriorityURL, false );
	mPriorityURL = *it;
	Cache::instance()->setPriorityURL( mPriorityURL, true ); // make sure it will stay in the cache
	connect( mPrefetch, SIGNAL( imageLoaded( bool )), SLOT( prefetchDone()));
}

void SlideShow::prefetchDone() {
	if( mPrefetch != NULL ) { 
		// don't call Cache::setPriorityURL( ... , false ) here - it will still take
		// a short while to reuse the image from the cache
		mPrefetch->release( this );
		mPrefetch = NULL;
		// prefetching completed and delay has already elapsed
		if( mStarted && !mTimer->isActive()) {
			slotTimeout();
		}
	}
}

//-Configuration--------------------------------------------
void SlideShow::readConfig(KConfig* config,const QString& group) {
	config->setGroup(group);
	mDelay=config->readNumEntry(CONFIG_DELAY,10);
	mDelaySuffix=config->readEntry(CONFIG_DELAY_SUFFIX,"s");
	mLoop=config->readBoolEntry(CONFIG_LOOP,false);
	mFullscreen=config->readBoolEntry(CONFIG_START_FULLSCREEN,true);
	mStopAtEnd=config->readBoolEntry(CONFIG_STOP_AT_END,true);
	mRandom=config->readBoolEntry(CONFIG_RANDOM,false);
	
	mRandom=GVConfig::self()->slideShowRandom();
}


void SlideShow::writeConfig(KConfig* config,const QString& group) const {
	config->setGroup(group);
	config->writeEntry(CONFIG_DELAY,mDelay);
	config->writeEntry(CONFIG_DELAY_SUFFIX,mDelaySuffix);
	config->writeEntry(CONFIG_LOOP,mLoop);
	config->writeEntry(CONFIG_START_FULLSCREEN,mFullscreen);
	config->writeEntry(CONFIG_STOP_AT_END,mStopAtEnd);
	config->writeEntry(CONFIG_RANDOM,mRandom);
	
	GVConfig::self()->setSlideShowRandom(mRandom);
}

} // namespace
