/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: http://www.qt-project.org/
**
** This file is part of the demonstration applications of the Qt Toolkit.
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

#include "plugin.h"
#include "qetcprovider.h"

#include <QDeclarativeEngine>
#include <qdebug.h>

QT_BEGIN_NAMESPACE

class QEtcDummyObject : public QObject
{
public:
    QEtcDummyObject() {}
};

void EtcProviderPlugin::registerTypes(const char *uri)
{
    //### this is required or "import Qt.labs.etcprovider 1.0" will give errors
    //### this plugin should eventually be replaced by a non-import type plugin
    //    (once it is available)
    qmlRegisterType<QEtcDummyObject>(uri,1,0,"EtcObject");
}

void EtcProviderPlugin::initializeEngine(QDeclarativeEngine *engine, const char *uri)
{
    qDebug () << uri;
    engine->addImageProvider(QLatin1String("etc"), new QEtcProvider());
}

QT_END_NAMESPACE

Q_EXPORT_PLUGIN2(qmletcproviderplugin, QT_PREPEND_NAMESPACE(EtcProviderPlugin))
