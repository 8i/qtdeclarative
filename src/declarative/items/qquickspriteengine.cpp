/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Declarative module of the Qt Toolkit.
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

#include "qquickspriteengine_p.h"
#include "qquicksprite_p.h"
#include <QDebug>
#include <QPainter>
#include <QSet>
#include <QtGui>

QT_BEGIN_NAMESPACE

/* TODO: Split out image logic from stochastic state logic
   Also make sharable
   Also solve the state data initialization/transfer issue so as to not need to make friends
*/

QQuickStochasticEngine::QQuickStochasticEngine(QObject *parent) :
    QObject(parent), m_timeOffset(0)
{
    //Default size 1
    setCount(1);
    m_advanceTime.start();
}

QQuickStochasticEngine::QQuickStochasticEngine(QList<QQuickStochasticState*> states, QObject *parent) :
    QObject(parent), m_states(states), m_timeOffset(0)
{
    //Default size 1
    setCount(1);
    m_advanceTime.start();
}

QQuickStochasticEngine::~QQuickStochasticEngine()
{
}

QQuickSpriteEngine::QQuickSpriteEngine(QObject *parent)
    : QQuickStochasticEngine(parent)
{
}

QQuickSpriteEngine::QQuickSpriteEngine(QList<QQuickSprite*> sprites, QObject *parent)
    : QQuickStochasticEngine(parent)
{
    foreach (QQuickSprite* sprite, sprites)
        m_states << (QQuickStochasticState*)sprite;
}

QQuickSpriteEngine::~QQuickSpriteEngine()
{
}


int QQuickSpriteEngine::maxFrames()
{
    return m_maxFrames;
}

/* States too large to fit in one row are split into multiple rows
   This is more efficient for the implementation, but should remain an implementation detail (invisible from QML)
   Therefore the below functions abstract sprite from the viewpoint of classes that pass the details onto shaders
   But States maintain their listed index for internal structures
TODO: All these calculations should be pre-calculated and cached during initialization for a significant performance boost
TODO: Above idea needs to have the varying duration offset added to it
*/
int QQuickSpriteEngine::spriteState(int sprite)
{
    int state = m_things[sprite];
    if (!m_sprites[state]->m_generatedCount)
        return state;
    int rowDuration = m_duration[sprite] * m_sprites[state]->m_framesPerRow;
    int extra = (m_timeOffset - m_startTimes[sprite])/rowDuration;
    return state + extra;
}

int QQuickSpriteEngine::spriteStart(int sprite)
{
    int state = m_things[sprite];
    if (!m_sprites[state]->m_generatedCount)
        return m_startTimes[sprite];
    int rowDuration = m_duration[sprite] * m_sprites[state]->m_framesPerRow;
    int extra = (m_timeOffset - m_startTimes[sprite])/rowDuration;
    return state + extra*rowDuration;
}

int QQuickSpriteEngine::spriteFrames(int sprite)
{
    int state = m_things[sprite];
    if (!m_sprites[state]->m_generatedCount)
        return m_sprites[state]->frames();
    int rowDuration = m_duration[sprite] * m_sprites[state]->m_framesPerRow;
    int extra = (m_timeOffset - m_startTimes[sprite])/rowDuration;
    if (extra == m_sprites[state]->m_generatedCount - 1)//last state
        return m_sprites[state]->frames() % m_sprites[state]->m_framesPerRow;
    else
        return m_sprites[state]->m_framesPerRow;
}

int QQuickSpriteEngine::spriteDuration(int sprite)
{
    int state = m_things[sprite];
    if (!m_sprites[state]->m_generatedCount)
        return m_duration[sprite];
    int rowDuration = m_duration[sprite] * m_sprites[state]->m_framesPerRow;
    int extra = (m_timeOffset - m_startTimes[sprite])/rowDuration;
    if (extra == m_sprites[state]->m_generatedCount - 1)//last state
        return (m_duration[sprite] * m_sprites[state]->frames()) % rowDuration;
    else
        return rowDuration;
}

int QQuickSpriteEngine::spriteY(int sprite)
{
    int state = m_things[sprite];
    if (!m_sprites[state]->m_generatedCount)
        return m_sprites[state]->m_rowY;
    int rowDuration = m_duration[sprite] * m_sprites[state]->m_framesPerRow;
    int extra = (m_timeOffset - m_startTimes[sprite])/rowDuration;
    return m_sprites[state]->m_rowY + m_sprites[state]->m_frameHeight * extra;
}

int QQuickSpriteEngine::spriteWidth(int sprite)
{
    int state = m_things[sprite];
    return m_sprites[state]->m_frameWidth;
}

int QQuickSpriteEngine::spriteHeight(int sprite)
{
    int state = m_things[sprite];
    return m_sprites[state]->m_frameHeight;
}

int QQuickSpriteEngine::spriteCount()//TODO: Actually image state count, need to rename these things to make sense together
{
    return m_imageStateCount;
}

void QQuickStochasticEngine::setGoal(int state, int sprite, bool jump)
{
    if (sprite >= m_things.count() || state >= m_states.count())
        return;
    if (!jump){
        m_goals[sprite] = state;
        return;
    }

    if (m_things[sprite] == state)
        return;//Already there
    m_things[sprite] = state;
    m_duration[sprite] = m_states[state]->variedDuration();
    m_goals[sprite] = -1;
    restart(sprite);
    emit stateChanged(sprite);
    emit m_states[state]->entered();
    return;
}

QImage QQuickSpriteEngine::assembledImage()
{
    int h = 0;
    int w = 0;
    m_maxFrames = 0;
    m_imageStateCount = 0;
    int maxSize = 0;

    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxSize);
    foreach (QQuickStochasticState* s, m_states){
        QQuickSprite* sprite = qobject_cast<QQuickSprite*>(s);
        if (sprite)
            m_sprites << sprite;
        else
            qDebug() << "Error: Non-sprite in QQuickSpriteEngine";
    }

    foreach (QQuickSprite* state, m_sprites){
        if (state->frames() > m_maxFrames)
            m_maxFrames = state->frames();

        QImage img(state->source().toLocalFile());
        if (img.isNull()) {
            qWarning() << "SpriteEngine: loading image failed..." << state->source().toLocalFile();
            return QImage();
        }

        //Check that the frame sizes are the same within one engine
        if (!state->m_frameWidth)
            state->m_frameWidth = img.width() / state->frames();

        if (!state->m_frameHeight)
            state->m_frameHeight = img.height();

        if (state->frames() * state->frameWidth() > maxSize){
            struct helper{
                static int divRoundUp(int a, int b){return (a+b-1)/b;}
            };
            int rowsNeeded = helper::divRoundUp(state->frames(), helper::divRoundUp(maxSize, state->frameWidth()));
            if (rowsNeeded * state->frameHeight() > maxSize){
                qWarning() << "SpriteEngine: Animation too large to fit in one texture..." << state->source().toLocalFile();
                qWarning() << "SpriteEngine: Your texture max size today is " << maxSize;
            }
            state->m_generatedCount = rowsNeeded;
            h += state->frameHeight() * rowsNeeded;
            w = qMax(w, helper::divRoundUp(maxSize, state->frameWidth()));
            m_imageStateCount += rowsNeeded;
        }else{
            h += state->frameHeight();
            w = qMax(w, state->frameWidth() * state->frames());
            m_imageStateCount++;
        }
    }

    //maxFrames is max number in a line of the texture
    QImage image(w, h, QImage::Format_ARGB32);
    image.fill(0);
    QPainter p(&image);
    int y = 0;
    foreach (QQuickSprite* state, m_sprites){
        QImage img(state->source().toLocalFile());
        int frameWidth = state->m_frameWidth;
        int frameHeight = state->m_frameHeight;
        if (img.height() == frameHeight && img.width() <  maxSize){//Simple case
            p.drawImage(0,y,img);
            state->m_rowY = y;
            y += frameHeight;
        }else{//Chopping up image case
            state->m_framesPerRow = image.width()/frameWidth;
            state->m_rowY = y;
            int x = 0;
            int curX = 0;
            int curY = 0;
            int framesLeft = state->frames();
            while (framesLeft > 0){
                if (image.width() - x + curX <= img.width()){//finish a row in image (dest)
                    int copied = image.width() - x;
                    Q_ASSERT(!(copied % frameWidth));//XXX: Just checking
                    framesLeft -= copied/frameWidth;
                    p.drawImage(x,y,img.copy(curX,curY,copied,frameHeight));
                    y += frameHeight;
                    curX += copied;
                    x = 0;
                    if (curX == img.width()){
                        curX = 0;
                        curY += frameHeight;
                    }
                }else{//finish a row in img (src)
                    int copied = img.width() - curX;
                    Q_ASSERT(!(copied % frameWidth));//XXX: Just checking
                    framesLeft -= copied/frameWidth;
                    p.drawImage(x,y,img.copy(curX,curY,copied,frameHeight));
                    curY += frameHeight;
                    x += copied;
                    curX = 0;
                }
            }
            if (x)
                y += frameHeight;
        }
    }

    if (image.height() > maxSize){
        qWarning() << "SpriteEngine: Too many animations to fit in one texture...";
        qWarning() << "SpriteEngine: Your texture max size today is " << maxSize;
        return QImage();
    }
    return image;
}

void QQuickStochasticEngine::setCount(int c)
{
    m_things.resize(c);
    m_goals.resize(c);
    m_duration.resize(c);
    m_startTimes.resize(c);
}

void QQuickStochasticEngine::start(int index, int state)
{
    if (index >= m_things.count())
        return;
    m_things[index] = state;
    m_duration[index] = m_states[state]->variedDuration();
    m_goals[index] = -1;
    restart(index);
}

void QQuickStochasticEngine::stop(int index)
{
    if (index >= m_things.count())
        return;
    //Will never change until start is called again with a new state - this is not a 'pause'
    for (int i=0; i<m_stateUpdates.count(); i++)
        m_stateUpdates[i].second.removeAll(index);
}

void QQuickStochasticEngine::restart(int index)
{
    m_startTimes[index] = m_timeOffset + m_advanceTime.elapsed();
    int time = m_duration[index] * m_states[m_things[index]]->frames() + m_startTimes[index];
    for (int i=0; i<m_stateUpdates.count(); i++)
        m_stateUpdates[i].second.removeAll(index);
    addToUpdateList(time, index);
}

uint QQuickStochasticEngine::updateSprites(uint time)//### would returning a list of changed idxs be faster than signals?
{
    //Sprite State Update;
    QSet<int> changedIndexes;
    while (!m_stateUpdates.isEmpty() && time >= m_stateUpdates.first().first){
        foreach (int idx, m_stateUpdates.first().second){
            if (idx >= m_things.count())
                continue;//TODO: Proper fix(because this does happen and I'm just ignoring it)
            int stateIdx = m_things[idx];
            int nextIdx = -1;
            int goalPath = goalSeek(stateIdx, idx);
            if (goalPath == -1){//Random
                qreal r =(qreal) qrand() / (qreal) RAND_MAX;
                qreal total = 0.0;
                for (QVariantMap::const_iterator iter=m_states[stateIdx]->m_to.constBegin();
                    iter!=m_states[stateIdx]->m_to.constEnd(); iter++)
                    total += (*iter).toReal();
                r*=total;
                for (QVariantMap::const_iterator iter= m_states[stateIdx]->m_to.constBegin();
                        iter!=m_states[stateIdx]->m_to.constEnd(); iter++){
                    if (r < (*iter).toReal()){
                        bool superBreak = false;
                        for (int i=0; i<m_states.count(); i++){
                            if (m_states[i]->name() == iter.key()){
                                nextIdx = i;
                                superBreak = true;
                                break;
                            }
                        }
                        if (superBreak)
                            break;
                    }
                    r -= (*iter).toReal();
                }
            }else{//Random out of shortest paths to goal
                nextIdx = goalPath;
            }
            if (nextIdx == -1)//No to states means stay here
                nextIdx = stateIdx;

            m_things[idx] = nextIdx;
            m_duration[idx] = m_states[nextIdx]->variedDuration();
            m_startTimes[idx] = time;
            if (nextIdx != stateIdx){
                changedIndexes << idx;
                emit m_states[nextIdx]->entered();
            }
            addToUpdateList((m_duration[idx] * m_states[nextIdx]->frames()) + time, idx);
        }
        m_stateUpdates.pop_front();
    }

    m_timeOffset = time;
    m_advanceTime.start();
    //TODO: emit this when a psuedostate changes too
    foreach (int idx, changedIndexes){//Batched so that update list doesn't change midway
        emit stateChanged(idx);
    }
    if (m_stateUpdates.isEmpty())
        return -1;
    return m_stateUpdates.first().first;
}

int QQuickStochasticEngine::goalSeek(int curIdx, int spriteIdx, int dist)
{
    QString goalName;
    if (m_goals[spriteIdx] != -1)
        goalName = m_states[m_goals[spriteIdx]]->name();
    else
        goalName = m_globalGoal;
    if (goalName.isEmpty())
        return -1;
    //TODO: caching instead of excessively redoing iterative deepening (which was chosen arbitarily anyways)
    // Paraphrased - implement in an *efficient* manner
    for (int i=0; i<m_states.count(); i++)
        if (m_states[curIdx]->name() == goalName)
            return curIdx;
    if (dist < 0)
        dist = m_states.count();
    QQuickStochasticState* curState = m_states[curIdx];
    for (QVariantMap::const_iterator iter = curState->m_to.constBegin();
        iter!=curState->m_to.constEnd(); iter++){
        if (iter.key() == goalName)
            for (int i=0; i<m_states.count(); i++)
                if (m_states[i]->name() == goalName)
                    return i;
    }
    QSet<int> options;
    for (int i=1; i<dist; i++){
        for (QVariantMap::const_iterator iter = curState->m_to.constBegin();
            iter!=curState->m_to.constEnd(); iter++){
            int option = -1;
            for (int j=0; j<m_states.count(); j++)//One place that could be a lot more efficient...
                if (m_states[j]->name() == iter.key())
                    if (goalSeek(j, spriteIdx, i) != -1)
                        option = j;
            if (option != -1)
                options << option;
        }
        if (!options.isEmpty()){
            if (options.count()==1)
                return *(options.begin());
            int option = -1;
            qreal r =(qreal) qrand() / (qreal) RAND_MAX;
            qreal total = 0;
            for (QSet<int>::const_iterator iter=options.constBegin();
                iter!=options.constEnd(); iter++)
                total += curState->m_to.value(m_states[(*iter)]->name()).toReal();
            r *= total;
            for (QVariantMap::const_iterator iter = curState->m_to.constBegin();
                iter!=curState->m_to.constEnd(); iter++){
                bool superContinue = true;
                for (int j=0; j<m_states.count(); j++)
                    if (m_states[j]->name() == iter.key())
                        if (options.contains(j))
                            superContinue = false;
                if (superContinue)
                    continue;
                if (r < (*iter).toReal()){
                    bool superBreak = false;
                    for (int j=0; j<m_states.count(); j++){
                        if (m_states[j]->name() == iter.key()){
                            option = j;
                            superBreak = true;
                            break;
                        }
                    }
                    if (superBreak)
                        break;
                }
                r-=(*iter).toReal();
            }
            return option;
        }
    }
    return -1;
}

void QQuickStochasticEngine::addToUpdateList(uint t, int idx)
{
    for (int i=0; i<m_stateUpdates.count(); i++){
        if (m_stateUpdates[i].first==t){
            m_stateUpdates[i].second << idx;
            return;
        }else if (m_stateUpdates[i].first > t){
            QList<int> tmpList;
            tmpList << idx;
            m_stateUpdates.insert(i, qMakePair(t, tmpList));
            return;
        }
    }
    QList<int> tmpList;
    tmpList << idx;
    m_stateUpdates << qMakePair(t, tmpList);
}

QT_END_NAMESPACE
