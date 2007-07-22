/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/
#ifndef SELECTIONCONTEXTMANAGERITEM_H
#define SELECTIONCONTEXTMANAGERITEM_H

// Qt
#include <QList>
#include <QObject>

// Local
#include "abstractcontextmanageritem.h"

class KFileItem;

namespace Gwenview {

class SelectionContextManagerItemPrivate;

class SelectionContextManagerItem : public AbstractContextManagerItem {
	Q_OBJECT
public:
	SelectionContextManagerItem(ContextManager*);
	~SelectionContextManagerItem();

	virtual void setImageView(ImageViewPart*);
	virtual void setSideBar(SideBar* sideBar);

private Q_SLOTS:
	void updatePreview();
	void updateSideBarContent();

private:
	void fillOneFileGroup(const KFileItem& item);

	void fillMultipleItemsGroup(const QList<KFileItem>& itemList);

	SelectionContextManagerItemPrivate * const d;
};

} // namespace

#endif /* SELECTIONCONTEXTMANAGERITEM_H */
