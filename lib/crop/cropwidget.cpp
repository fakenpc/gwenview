// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau <agateau@kde.org>

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

*/
// Self

// Qt
#include <QApplication>
#include <QDesktopWidget>
#include <QPushButton>
#include <QtMath>
#include <QDebug>
#include <QLineEdit>

// KDE
#include <KLocalizedString>

// Local
#include <lib/documentview/rasterimageview.h>
#include <QFontDatabase>
#include "croptool.h"
#include "signalblocker.h"
#include "ui_cropwidget.h"
#include "cropwidget.h"

namespace Gwenview
{

// Euclidean algorithm to compute the greatest common divisor of two integers.
// Found at:
// http://en.wikipedia.org/wiki/Euclidean_algorithm
static int gcd(int a, int b)
{
    return b == 0 ? a : gcd(b, a % b);
}

static QSize ratio(const QSize &size)
{
    const int divisor = gcd(size.width(), size.height());
    return size / divisor;
}

struct CropWidgetPrivate : public Ui_CropWidget
{
    CropWidget* q;

    Document::Ptr mDocument;
    CropTool* mCropTool;
    bool mUpdatingFromCropTool;
    int mCurrentImageComboBoxIndex;
    int mCropRatioComboBoxCurrentIndex;

    bool ratioIsConstrained() const
    {
        return cropRatio() > 0;
    }

    QSizeF chosenRatio() const
    {
        // A size of 0 represents no ratio, i.e. the combobox is empty
        if (ratioComboBox->currentText().isEmpty()) {
            return QSizeF(0, 0);
        }

        // A preset ratio is selected
        const int index = ratioComboBox->currentIndex();
        if (index != -1 && ratioComboBox->currentText() == ratioComboBox->itemText(index)) {
            return ratioComboBox->currentData().toSizeF();
        }

        // A custom ratio has been entered, extract ratio from the text
        // If invalid, return zero size instead
        const QStringList lst = ratioComboBox->currentText().split(':');
        if (lst.size() != 2) {
            return QSizeF(0, 0);
        }
        bool ok;
        const double width = lst[0].toDouble(&ok);
        if (!ok) {
            return QSizeF(0, 0);
        }
        const double height = lst[1].toDouble(&ok);
        if (!ok) {
            return QSizeF(0, 0);
        }

        // Valid custom value
        return QSizeF(width, height);
    }

    void setChosenRatio(QSizeF size) const
    {
        // Size matches preset ratio, let's set the combobox to that
        const int index = ratioComboBox->findData(size);
        if (index >= 0) {
            ratioComboBox->setCurrentIndex(index);
            return;
        }

        // Deselect whatever was selected if anything
        ratioComboBox->setCurrentIndex(-1);

        // If size is 0 (represents blank combobox, i.e., unrestricted)
        if (size.isEmpty()) {
            ratioComboBox->clearEditText();
            return;
        }

        // Size must be custom ratio, convert to text and add to combobox
        QString ratioString = QString("%1:%2").arg(size.width()).arg(size.height());
        ratioComboBox->setCurrentText(ratioString);
    }

    double cropRatio() const
    {
        if (q->advancedSettingsEnabled()) {
            QSizeF size = chosenRatio();
            if (size.isEmpty()) {
                return 0;
            }
            return size.height() / size.width();
        }

        if (q->restrictToImageRatio()) {
            QSizeF size = ratio(mDocument->size());
            return size.height() / size.width();
        }

        return 0;
    }

    void addRatioToComboBox(const QSizeF& size, const QString& label = QString())
    {
        QString text = label.isEmpty()
            ? QString("%1:%2").arg(size.width()).arg(size.height())
            : label;
        ratioComboBox->addItem(text, QVariant(size));
    }

    void addSectionHeaderToComboBox(const QString& title)
    {
        // Insert a line
        ratioComboBox->insertSeparator(ratioComboBox->count());

        // Insert our section header
        // This header is made of a separator with a text. We reset
        // Qt::AccessibleDescriptionRole to the header text otherwise QComboBox
        // delegate will draw a separator line instead of our text.
        int index = ratioComboBox->count();
        ratioComboBox->insertSeparator(index);
        ratioComboBox->setItemText(index, title);
        ratioComboBox->setItemData(index, title, Qt::AccessibleDescriptionRole);
        ratioComboBox->setItemData(index, Qt::AlignHCenter, Qt::TextAlignmentRole);
    }

    void initRatioComboBox()
    {
        QList<QSizeF> ratioList;
        const qreal sqrt2 = qSqrt(2.);
        ratioList
                << QSizeF(16, 9)
                << QSizeF(7, 5)
                << QSizeF(3, 2)
                << QSizeF(4, 3)
                << QSizeF(5, 4);

        addRatioToComboBox(ratio(mDocument->size()), i18n("Current Image"));
        mCurrentImageComboBoxIndex = ratioComboBox->count() - 1; // We need to refer to this ratio later

        addRatioToComboBox(QSizeF(1, 1), i18n("Square"));
        addRatioToComboBox(ratio(QApplication::desktop()->screenGeometry().size()), i18n("This Screen"));
        addSectionHeaderToComboBox(i18n("Landscape"));

        Q_FOREACH(const QSizeF& size, ratioList) {
            addRatioToComboBox(size);
        }
        addRatioToComboBox(QSizeF(sqrt2, 1), i18n("ISO (A4, A3...)"));
        addRatioToComboBox(QSizeF(11, 8.5), i18n("US Letter"));
        addSectionHeaderToComboBox(i18n("Portrait"));
        Q_FOREACH(QSizeF size, ratioList) {
            size.transpose();
            addRatioToComboBox(size);
        }
        addRatioToComboBox(QSizeF(1, sqrt2), i18n("ISO (A4, A3...)"));
        addRatioToComboBox(QSizeF(8.5, 11), i18n("US Letter"));

        ratioComboBox->setMaxVisibleItems(ratioComboBox->count());
        ratioComboBox->clearEditText();

        QLineEdit* edit = qobject_cast<QLineEdit*>(ratioComboBox->lineEdit());
        Q_ASSERT(edit);
        // Do not use i18n("%1:%2") because ':' should not be translated, it is
        // used to parse the ratio string.
        edit->setPlaceholderText(QString("%1:%2").arg(i18n("Width")).arg(i18n("Height")));

        // Enable clear button
        edit->setClearButtonEnabled(true);
        // Must manually adjust minimum width because the auto size adjustment doesn't take the
        // clear button into account
        const int width = ratioComboBox->minimumSizeHint().width();
        ratioComboBox->setMinimumWidth(width + 24);

        ratioComboBox->setCurrentIndex(-1);
    }

    QRect cropRect() const
    {
        QRect rect(
            leftSpinBox->value(),
            topSpinBox->value(),
            widthSpinBox->value(),
            heightSpinBox->value()
        );
        return rect;
    }

    void initSpinBoxes()
    {
        QSize size = mDocument->size();
        leftSpinBox->setMaximum(size.width());
        widthSpinBox->setMaximum(size.width());
        topSpinBox->setMaximum(size.height());
        heightSpinBox->setMaximum(size.height());
    }

    void initDialogButtonBox()
    {
        QPushButton* cropButton = dialogButtonBox->button(QDialogButtonBox::Ok);
        cropButton->setIcon(QIcon::fromTheme("transform-crop-and-resize"));
        cropButton->setText(i18n("Crop"));

        QObject::connect(dialogButtonBox, &QDialogButtonBox::accepted, q, &CropWidget::cropRequested);
        QObject::connect(dialogButtonBox, &QDialogButtonBox::rejected, q, &CropWidget::done);
    }
};

CropWidget::CropWidget(QWidget* parent, RasterImageView* imageView, CropTool* cropTool)
: QWidget(parent)
, d(new CropWidgetPrivate)
{
    setWindowFlags(Qt::Tool);
    d->q = this;
    d->mDocument = imageView->document();
    d->mUpdatingFromCropTool = false;
    d->mCropTool = cropTool;
    d->setupUi(this);
    setFont(QFontDatabase::systemFont(QFontDatabase::SmallestReadableFont));
    layout()->setSizeConstraint(QLayout::SetFixedSize);

    connect(d->advancedCheckBox, &QCheckBox::toggled, this, &CropWidget::slotAdvancedCheckBoxToggled);
    d->advancedWidget->setVisible(false);
    d->advancedWidget->layout()->setMargin(0);

    connect(d->restrictToImageRatioCheckBox, &QCheckBox::toggled, this, &CropWidget::applyRatioConstraint);

    d->initRatioComboBox();

    connect(d->mCropTool, &CropTool::rectUpdated, this, &CropWidget::setCropRect);

    connect(d->leftSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &CropWidget::slotPositionChanged);
    connect(d->topSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &CropWidget::slotPositionChanged);
    connect(d->widthSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &CropWidget::slotWidthChanged);
    connect(d->heightSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &CropWidget::slotHeightChanged);

    d->initDialogButtonBox();

    // We need to listen for both signals because the combobox is multi-function:
    // Text Changed: required so that manual ratio entry is detected (index doesn't change)
    // Index Changed: required so that choosing an item with the same text is detected (e.g. going from US Letter portrait
    // to US Letter landscape)
    connect(d->ratioComboBox, &QComboBox::editTextChanged, this, &CropWidget::slotRatioComboBoxChanged);
    connect(d->ratioComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &CropWidget::slotRatioComboBoxChanged);

    // Don't do this before signals are connected, otherwise the tool won't get
    // initialized
    d->initSpinBoxes();

    setCropRect(d->mCropTool->rect());
}

CropWidget::~CropWidget()
{
    delete d;
}

void CropWidget::setAdvancedSettingsEnabled(bool enable)
{
    d->advancedCheckBox->setChecked(enable);
}

bool CropWidget::advancedSettingsEnabled() const
{
    return d->advancedCheckBox->isChecked();
}

void CropWidget::setRestrictToImageRatio(bool restrict)
{
    d->restrictToImageRatioCheckBox->setChecked(restrict);
}

bool CropWidget::restrictToImageRatio() const
{
    return d->restrictToImageRatioCheckBox->isChecked();
}

void CropWidget::setCropRatio(QSizeF size)
{
    d->setChosenRatio(size);
}

QSizeF CropWidget::cropRatio() const
{
    return d->chosenRatio();
}

void CropWidget::setCropRatioIndex(int index)
{
    d->ratioComboBox->setCurrentIndex(index);
}

int CropWidget::cropRatioIndex() const
{
    return d->mCropRatioComboBoxCurrentIndex;
}

void CropWidget::setCropRect(const QRect& rect)
{
    d->mUpdatingFromCropTool = true;
    d->leftSpinBox->setValue(rect.left());
    d->topSpinBox->setValue(rect.top());
    d->widthSpinBox->setValue(rect.width());
    d->heightSpinBox->setValue(rect.height());
    d->mUpdatingFromCropTool = false;
}

void CropWidget::slotPositionChanged()
{
    const QSize size = d->mDocument->size();
    d->widthSpinBox->setMaximum(size.width() - d->leftSpinBox->value());
    d->heightSpinBox->setMaximum(size.height() - d->topSpinBox->value());

    if (d->mUpdatingFromCropTool) {
        return;
    }
    d->mCropTool->setRect(d->cropRect());
}

void CropWidget::slotWidthChanged()
{
    d->leftSpinBox->setMaximum(d->mDocument->width() - d->widthSpinBox->value());

    if (d->mUpdatingFromCropTool) {
        return;
    }
    if (d->ratioIsConstrained()) {
        int height = int(d->widthSpinBox->value() * d->cropRatio());
        d->heightSpinBox->setValue(height);
    }
    d->mCropTool->setRect(d->cropRect());
}

void CropWidget::slotHeightChanged()
{
    d->topSpinBox->setMaximum(d->mDocument->height() - d->heightSpinBox->value());

    if (d->mUpdatingFromCropTool) {
        return;
    }
    if (d->ratioIsConstrained()) {
        int width = int(d->heightSpinBox->value() / d->cropRatio());
        d->widthSpinBox->setValue(width);
    }
    d->mCropTool->setRect(d->cropRect());
}

void CropWidget::applyRatioConstraint()
{
    double ratio = d->cropRatio();
    d->mCropTool->setCropRatio(ratio);

    if (!d->ratioIsConstrained()) {
        return;
    }
    QRect rect = d->cropRect();
    rect.setHeight(int(rect.width() * ratio));
    d->mCropTool->setRect(rect);
}

void CropWidget::slotAdvancedCheckBoxToggled(bool checked)
{
    d->advancedWidget->setVisible(checked);
    d->restrictToImageRatioCheckBox->setVisible(!checked);
    applyRatioConstraint();
}

void CropWidget::slotRatioComboBoxChanged()
{
    const QString text = d->ratioComboBox->currentText();

    // If text cleared, clear the current item as well
    if (text.isEmpty()) {
        d->ratioComboBox->setCurrentIndex(-1);
    }

    // We want to keep track of the selected ratio, including when the user has entered a custom ratio
    // or cleared the text. We can't simply use currentIndex() because this stays >= 0 when the user manually
    // enters text. We also can't set the current index to -1 when there is no match like above because that
    // interferes when manually entering text.
    // Furthermore, since there can be duplicate text items, we can't rely on findText() as it will stop on
    // the first match it finds. Therefore we must check if there's a match, and if so, get the index directly.
    if (d->ratioComboBox->findText(text) >= 0) {
        d->mCropRatioComboBoxCurrentIndex = d->ratioComboBox->currentIndex();
    } else {
        d->mCropRatioComboBoxCurrentIndex = -1;
    }

    applyRatioConstraint();
}

void CropWidget::updateCropRatio()
{
    // First we need to re-calculate the "Current Image" ratio in case the user rotated the image
    d->ratioComboBox->setItemData(d->mCurrentImageComboBoxIndex, QVariant(ratio(d->mDocument->size())));

    // Always re-apply the constraint, even though we only need to when the user has "Current Image"
    // selected or the "Restrict to current image" checked, since there's no harm
    applyRatioConstraint();
    // If the ratio is unrestricted, calling applyRatioConstraint doesn't update the rect, so we call
    // this manually to make sure the rect is adjusted to fit within the image
    d->mCropTool->setRect(d->mCropTool->rect());
}

} // namespace
