/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
** $QT_END_LICENSE$
**
****************************************************************************/
#include <QtCore/QAbstractListModel>
#include <QtCore/QDate>

class StockPrice : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QDate date READ date)
    Q_PROPERTY(qreal openPrice READ openPrice)
    Q_PROPERTY(qreal closePrice READ closePrice)
    Q_PROPERTY(qreal highPrice READ highPrice)
    Q_PROPERTY(qreal lowPrice READ lowPrice)
    Q_PROPERTY(qint32 volume READ volume)
    Q_PROPERTY(qreal adjustedPrice READ adjustedPrice)
public:

    StockPrice(QObject *parent = 0)
        : QObject(parent)
        , _openPrice(-1)
        , _closePrice(-1)
        , _highPrice(-1)
        , _lowPrice(-1)
        , _volume(-1)
        , _adjustedPrice(-1)
    {
    }
    QDate date() const {return _date;}
    qreal openPrice() const {return _openPrice; }
    qreal closePrice() const {return _closePrice;}
    qreal highPrice() const {return _highPrice;}
    qreal lowPrice() const{return _lowPrice;}
    qreal adjustedPrice() const{return _adjustedPrice;}
    qint32 volume() const{return _volume;}

    void setDate(const QDate& date){_date = date;}
    void setOpenPrice(qreal price){_openPrice = price;}
    void setClosePrice(qreal price){_closePrice = price;}
    void setHighPrice(qreal price){_highPrice = price;}
    void setLowPrice(qreal price){_lowPrice = price;}
    void setAdjustedPrice(qreal price) {_adjustedPrice = price;}
    void setVolume(qint32 volume) {_volume = volume;}

private:
    QDate _date;
    qreal _openPrice;
    qreal _closePrice;
    qreal _highPrice;
    qreal _lowPrice;
    qint32 _volume;
    qreal _adjustedPrice;
};

class QNetworkReply;
class QNetworkAccessManager;
class StockModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(QString stockName READ stockName WRITE setStockName NOTIFY stockNameChanged)
    Q_PROPERTY(QDate startDate READ startDate WRITE setStartDate NOTIFY startDateChanged)
    Q_PROPERTY(QDate endDate READ endDate WRITE setEndDate NOTIFY endDateChanged)
    Q_PROPERTY(StockDataCycle dataCycle READ dataCycle WRITE setDataCycle NOTIFY dataCycleChanged)

    Q_ENUMS(StockDataCycle)
public:
    enum StockDataCycle {
        Daily,
        Weekly,
        Monthly,
        Dividend
    };

    enum StockModelRoles {
        DateRole = Qt::UserRole + 1,
        SectionRole,
        OpenPriceRole,
        ClosePriceRole,
        HighPriceRole,
        LowPriceRole,
        VolumeRole,
        AdjustedPriceRole
    };

    StockModel(QObject *parent = 0);

    QString stockName() const;
    void setStockName(const QString& name);

    QDate startDate() const;
    void setStartDate(const QDate& date);

    QDate endDate() const;
    void setEndDate(const QDate& date);

    StockDataCycle dataCycle() const;
    void setDataCycle(StockDataCycle cycle);

    int rowCount(const QModelIndex & parent = QModelIndex()) const;

    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;

signals:
    void stockNameChanged();
    void startDateChanged();
    void endDateChanged();
    void dataCycleChanged();
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);

public slots:
    void requestData();
    StockPrice* stockPriceAtIndex(int idx) const;
private slots:
    void doRequest();
    void update(QNetworkReply* reply);
private:
    QString dataCycleString() const;
    QList<StockPrice*> _prices;
    QString _stockName;
    QDate _startDate;
    QDate _endDate;
    StockDataCycle _dataCycle;
    QNetworkAccessManager* _manager;
    bool _updating;
};




