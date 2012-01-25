/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: http://www.qt-project.org/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QPainter>
#include <QtQuick/QQuickView>
#include <QApplication>
#include <QEasingCurve>
#include <QtQuick/QQuickPaintedItem>

class EasingPlot : public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(QEasingCurve easing READ easing WRITE setEasing NOTIFY easingChanged);

public:
    EasingPlot();

    QEasingCurve easing() const;
    void setEasing(const QEasingCurve &);

signals:
    void easingChanged();

protected:
    virtual void paint(QPainter *painter);

private:
    QEasingCurve m_easing;
};

EasingPlot::EasingPlot()
{
}

QEasingCurve EasingPlot::easing() const
{
    return m_easing;
}

void EasingPlot::setEasing(const QEasingCurve &e)
{
    if (m_easing == e)
        return;

    m_easing = e;
    emit easingChanged();

    update();
}

void EasingPlot::paint(QPainter *painter)
{
    QPointF lastPoint(0, 0);

    for (int ii = 1; ii <= 100; ++ii) {
        qreal value = m_easing.valueForProgress(qreal(ii) / 100.);

        QPointF currentPoint(width() * qreal(ii) / 100., value * (height() - 1));
        painter->drawLine(lastPoint, currentPoint);

        lastPoint = currentPoint;
    }
}

int main(int argc, char ** argv)
{
    QApplication app(argc, argv);

    qmlRegisterType<EasingPlot>("EasingPlot", 1, 0, "EasingPlot");

    QQuickView view;
    view.setSource(QUrl("qrc:/easing.qml"));
    view.show();

    return app.exec();
}

#include "main.moc"
