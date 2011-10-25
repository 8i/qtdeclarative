/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
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

#include "qquickvisualdatamodel_p.h"
#include "qquickitem.h"

#include <QtCore/qcoreapplication.h>
#include <QtDeclarative/qdeclarativecontext.h>
#include <QtDeclarative/qdeclarativeengine.h>
#include <QtDeclarative/qdeclarativeexpression.h>
#include <QtDeclarative/qdeclarativeinfo.h>

#include <private/qdeclarativecontext_p.h>
#include <private/qdeclarativepackage_p.h>
#include <private/qdeclarativeopenmetaobject_p.h>
#include <private/qdeclarativelistaccessor_p.h>
#include <private/qdeclarativedata_p.h>
#include <private/qdeclarativepropertycache_p.h>
#include <private/qdeclarativeguard_p.h>
#include <private/qdeclarativeglobal_p.h>
#include <private/qmetaobjectbuilder_p.h>
#include <private/qdeclarativeproperty_p.h>
#include <private/qquickvisualadaptormodel_p.h>
#include <private/qdeclarativechangeset_p.h>
#include <private/qdeclarativelistcompositor_p.h>
#include <private/qdeclarativeengine_p.h>
#include <private/qobject_p.h>

#include <QtCore/qhash.h>
#include <QtCore/qlist.h>

QT_BEGIN_NAMESPACE

typedef QDeclarativeListCompositor Compositor;

class QQuickVisualDataGroupEmitter
{
public:
    virtual void emitModelUpdated(const QDeclarativeChangeSet &changeSet, bool reset) = 0;
    virtual void createdPackage(int, QDeclarativePackage *) {}
    virtual void destroyingPackage(QDeclarativePackage *) {}

    QIntrusiveListNode emitterNode;
};

typedef QIntrusiveList<QQuickVisualDataGroupEmitter, &QQuickVisualDataGroupEmitter::emitterNode> QQuickVisualDataGroupEmitterList;

//---------------------------------------------------------------------------

class QQuickVisualDataGroupPrivate : public QObjectPrivate
{
public:
    Q_DECLARE_PUBLIC(QQuickVisualDataGroup)

    QQuickVisualDataGroupPrivate() : group(Compositor::Cache), defaultInclude(false) {}

    static QQuickVisualDataGroupPrivate *get(QQuickVisualDataGroup *group) {
        return static_cast<QQuickVisualDataGroupPrivate *>(QObjectPrivate::get(group)); }

    void setModel(QQuickVisualDataModel *model, Compositor::Group group);
    void emitChanges(QV8Engine *engine);
    void emitModelUpdated(bool reset);

    void createdPackage(int index, QDeclarativePackage *package);
    void destroyingPackage(QDeclarativePackage *package);

    bool parseGroupArgs(QDeclarativeV8Function *args, int *index, int *count, int *groups) const;

    Compositor::Group group;
    QDeclarativeGuard<QQuickVisualDataModel> model;
    QQuickVisualDataGroupEmitterList emitters;
    QDeclarativeChangeSet changeSet;
    QString name;
    bool defaultInclude;
};

//---------------------------------------------------------------------------

class QQuickVisualDataModelCacheItem;
class QQuickVisualDataModelCacheMetaType;
class QQuickVisualDataModelParts;

class QQuickVisualDataModelPrivate : public QObjectPrivate, public QQuickVisualDataGroupEmitter
{
    Q_DECLARE_PUBLIC(QQuickVisualDataModel)
public:
    QQuickVisualDataModelPrivate(QDeclarativeContext *);

    static QQuickVisualDataModelPrivate *get(QQuickVisualDataModel *m) {
        return static_cast<QQuickVisualDataModelPrivate *>(QObjectPrivate::get(m));
    }

    void init();
    void connectModel(QQuickVisualAdaptorModel *model);

    QObject *object(Compositor::Group group, int index, bool complete, bool reference);
    void destroy(QObject *object);
    QQuickVisualDataModel::ReleaseFlags release(QObject *object);
    QString stringValue(Compositor::Group group, int index, const QString &name);
    int cacheIndexOf(QObject *object) const;
    void emitCreatedPackage(Compositor::iterator at, QDeclarativePackage *package);
    void emitCreatedItem(Compositor::iterator at, QQuickItem *item) {
        emit q_func()->createdItem(at.index[m_compositorGroup], item); }
    void emitDestroyingPackage(QDeclarativePackage *package);
    void emitDestroyingItem(QQuickItem *item) { emit q_func()->destroyingItem(item); }

    void updateFilterGroup();

    void addGroups(Compositor::Group group, int index, int count, int groupFlags);
    void removeGroups(Compositor::Group group, int index, int count, int groupFlags);
    void setGroups(Compositor::Group group, int index, int count, int groupFlags);

    void itemsInserted(
            const QVector<Compositor::Insert> &inserts,
            QVarLengthArray<QVector<QDeclarativeChangeSet::Insert>, Compositor::MaximumGroupCount> *translatedInserts,
            QHash<int, QList<QQuickVisualDataModelCacheItem *> > *movedItems = 0);
    void itemsInserted(const QVector<Compositor::Insert> &inserts);
    void itemsRemoved(
            const QVector<Compositor::Remove> &removes,
            QVarLengthArray<QVector<QDeclarativeChangeSet::Remove>, Compositor::MaximumGroupCount> *translatedRemoves,
            QHash<int, QList<QQuickVisualDataModelCacheItem *> > *movedItems = 0);
    void itemsRemoved(const QVector<Compositor::Remove> &removes);
    void itemsMoved(
            const QVector<Compositor::Remove> &removes, const QVector<Compositor::Insert> &inserts);
    void itemsChanged(const QVector<Compositor::Change> &changes);
    template <typename T> static v8::Local<v8::Array> buildChangeList(const QVector<T> &changes);
    void emitChanges();
    void emitModelUpdated(const QDeclarativeChangeSet &changeSet, bool reset);


    static void group_append(QDeclarativeListProperty<QQuickVisualDataGroup> *property, QQuickVisualDataGroup *group);
    static int group_count(QDeclarativeListProperty<QQuickVisualDataGroup> *property);
    static QQuickVisualDataGroup *group_at(QDeclarativeListProperty<QQuickVisualDataGroup> *property, int index);

    QQuickVisualAdaptorModel *m_adaptorModel;
    QDeclarativeComponent *m_delegate;
    QQuickVisualDataModelCacheMetaType *m_cacheMetaType;
    QDeclarativeGuard<QDeclarativeContext> m_context;

    QList<QQuickVisualDataModelCacheItem *> m_cache;
    QQuickVisualDataModelParts *m_parts;
    QQuickVisualDataGroupEmitterList m_pendingParts;

    QDeclarativeListCompositor m_compositor;
    QDeclarativeListCompositor::Group m_compositorGroup;
    bool m_complete : 1;
    bool m_delegateValidated : 1;
    bool m_completePending : 1;
    bool m_reset : 1;
    bool m_transaction : 1;

    QString m_filterGroup;
    QList<QByteArray> watchedRoles;

    union {
        struct {
            QQuickVisualDataGroup *m_cacheItems;
            QQuickVisualDataGroup *m_items;
            QQuickVisualDataGroup *m_persistedItems;
        };
        QQuickVisualDataGroup *m_groups[Compositor::MaximumGroupCount];
    };
    int m_groupCount;
};

//---------------------------------------------------------------------------

class QQuickVisualPartsModel : public QQuickVisualModel, public QQuickVisualDataGroupEmitter
{
    Q_OBJECT
    Q_PROPERTY(QString filterOnGroup READ filterGroup WRITE setFilterGroup NOTIFY filterGroupChanged RESET resetFilterGroup)
public:
    QQuickVisualPartsModel(QQuickVisualDataModel *model, const QString &part, QObject *parent = 0);
    ~QQuickVisualPartsModel();

    QString filterGroup() const;
    void setFilterGroup(const QString &group);
    void resetFilterGroup();
    void updateFilterGroup();
    void updateFilterGroup(Compositor::Group group, const QDeclarativeChangeSet &changeSet);

    int count() const;
    bool isValid() const;
    QQuickItem *item(int index, bool complete=true);
    ReleaseFlags release(QQuickItem *item);
    bool completePending() const;
    void completeItem();
    QString stringValue(int index, const QString &role);
    void setWatchedRoles(QList<QByteArray> roles);

    int indexOf(QQuickItem *item, QObject *objectContext) const;

    void emitModelUpdated(const QDeclarativeChangeSet &changeSet, bool reset);

    void createdPackage(int index, QDeclarativePackage *package);
    void destroyingPackage(QDeclarativePackage *package);

Q_SIGNALS:
    void filterGroupChanged();

private:
    QQuickVisualDataModel *m_model;
    QHash<QObject *, QDeclarativePackage *> m_packaged;
    QString m_part;
    QString m_filterGroup;
    QList<QByteArray> m_watchedRoles;
    Compositor::Group m_compositorGroup;
    bool m_inheritGroup;
};

class QQuickVisualDataModelPartsMetaObject : public QDeclarativeOpenMetaObject
{
public:
    QQuickVisualDataModelPartsMetaObject(QObject *parent)
    : QDeclarativeOpenMetaObject(parent) {}

    virtual void propertyCreated(int, QMetaPropertyBuilder &);
    virtual QVariant initialValue(int);
};

class QQuickVisualDataModelParts : public QObject
{
Q_OBJECT
public:
    QQuickVisualDataModelParts(QQuickVisualDataModel *parent);

    QQuickVisualDataModel *model;
    QList<QQuickVisualPartsModel *> models;
};

void QQuickVisualDataModelPartsMetaObject::propertyCreated(int, QMetaPropertyBuilder &prop)
{
    prop.setWritable(false);
}

QVariant QQuickVisualDataModelPartsMetaObject::initialValue(int id)
{
    QQuickVisualDataModelParts *parts = static_cast<QQuickVisualDataModelParts *>(object());
    QQuickVisualPartsModel *m = new QQuickVisualPartsModel(
            parts->model, QString::fromUtf8(name(id)), parts);
    parts->models.append(m);
    return QVariant::fromValue(static_cast<QObject *>(m));
}

QQuickVisualDataModelParts::QQuickVisualDataModelParts(QQuickVisualDataModel *parent)
: QObject(parent), model(parent)
{
    new QQuickVisualDataModelPartsMetaObject(this);
}

//---------------------------------------------------------------------------

class QQuickVisualDataModelCacheMetaType : public QDeclarativeRefCount
{
public:
    QQuickVisualDataModelCacheMetaType(QV8Engine *engine, QQuickVisualDataModel *model, const QStringList &groupNames);
    ~QQuickVisualDataModelCacheMetaType();

    int parseGroups(const QStringList &groupNames) const;
    int parseGroups(QV8Engine *engine, const v8::Local<v8::Value> &groupNames) const;

    static v8::Handle<v8::Value> get_model(v8::Local<v8::String>, const v8::AccessorInfo &info);
    static v8::Handle<v8::Value> get_groups(v8::Local<v8::String>, const v8::AccessorInfo &info);
    static void set_groups(
            v8::Local<v8::String>, v8::Local<v8::Value> value, const v8::AccessorInfo &info);
    static v8::Handle<v8::Value> get_member(v8::Local<v8::String>, const v8::AccessorInfo &info);
    static void set_member(
            v8::Local<v8::String>, v8::Local<v8::Value> value, const v8::AccessorInfo &info);
    static v8::Handle<v8::Value> get_index(v8::Local<v8::String>, const v8::AccessorInfo &info);

    QDeclarativeGuard<QQuickVisualDataModel> model;
    const int groupCount;
    const int memberPropertyOffset;
    const int indexPropertyOffset;
    QV8Engine * const v8Engine;
    QMetaObject *metaObject;
    const QStringList groupNames;
    v8::Persistent<v8::Function> constructor;
};

class QQuickVisualDataModelCacheItem : public QV8ObjectResource
{
    V8_RESOURCE_TYPE(VisualDataItemType)
public:
    QQuickVisualDataModelCacheItem(QQuickVisualDataModelCacheMetaType *metaType)
        : QV8ObjectResource(metaType->v8Engine)
        , metaType(metaType)
        , object(0)
        , attached(0)
        , objectRef(0)
        , scriptRef(0)
        , groups(0)
    {
        metaType->addref();
    }

    ~QQuickVisualDataModelCacheItem()
    {
        Q_ASSERT(scriptRef == 0);
        Q_ASSERT(objectRef == 0);
        Q_ASSERT(!object);

        metaType->release();
    }

    void referenceObject() { ++objectRef; }
    bool releaseObject() { return --objectRef == 0 && !(groups & Compositor::PersistedFlag); }
    bool isObjectReferenced() const { return objectRef == 0 && !(groups & Compositor::PersistedFlag); }

    bool isReferenced() const { return objectRef || scriptRef || (groups & Compositor::PersistedFlag); }

    void Dispose();

    QQuickVisualDataModelCacheMetaType * const metaType;
    QDeclarativeGuard<QObject> object;
    QQuickVisualDataModelAttached *attached;
    int objectRef;
    int scriptRef;
    int groups;
    int index[Compositor::MaximumGroupCount];
};

class QQuickVisualDataModelAttachedMetaObject : public QAbstractDynamicMetaObject
{
public:
    QQuickVisualDataModelAttachedMetaObject(
            QQuickVisualDataModelAttached *attached, QQuickVisualDataModelCacheMetaType *metaType);
    ~QQuickVisualDataModelAttachedMetaObject();

    int metaCall(QMetaObject::Call, int _id, void **);

private:
    QQuickVisualDataModelAttached *attached;
    QQuickVisualDataModelCacheMetaType *metaType;
};

//---------------------------------------------------------------------------

QHash<QObject*, QQuickVisualDataModelAttached*> QQuickVisualDataModelAttached::attachedProperties;

/*!
    \qmlclass VisualDataModel QQuickVisualDataModel
    \inqmlmodule QtQuick 2
    \ingroup qml-working-with-data
    \brief The VisualDataModel encapsulates a model and delegate

    A VisualDataModel encapsulates a model and the delegate that will
    be instantiated for items in the model.

    It is usually not necessary to create VisualDataModel elements.
    However, it can be useful for manipulating and accessing the \l modelIndex
    when a QAbstractItemModel subclass is used as the
    model. Also, VisualDataModel is used together with \l Package to
    provide delegates to multiple views.

    The example below illustrates using a VisualDataModel with a ListView.

    \snippet doc/src/snippets/declarative/visualdatamodel.qml 0
*/

QQuickVisualDataModelPrivate::QQuickVisualDataModelPrivate(QDeclarativeContext *ctxt)
    : m_adaptorModel(0)
    , m_delegate(0)
    , m_cacheMetaType(0)
    , m_context(ctxt)
    , m_parts(0)
    , m_compositorGroup(Compositor::Cache)
    , m_complete(false)
    , m_delegateValidated(false)
    , m_completePending(false)
    , m_reset(false)
    , m_transaction(false)
    , m_filterGroup(QStringLiteral("items"))
    , m_cacheItems(0)
    , m_items(0)
    , m_groupCount(3)
{
}

void QQuickVisualDataModelPrivate::connectModel(QQuickVisualAdaptorModel *model)
{
    Q_Q(QQuickVisualDataModel);

    QObject::connect(model, SIGNAL(itemsInserted(int,int)), q, SLOT(_q_itemsInserted(int,int)));
    QObject::connect(model, SIGNAL(itemsRemoved(int,int)), q, SLOT(_q_itemsRemoved(int,int)));
    QObject::connect(model, SIGNAL(itemsMoved(int,int,int)), q, SLOT(_q_itemsMoved(int,int,int)));
    QObject::connect(model, SIGNAL(itemsChanged(int,int)), q, SLOT(_q_itemsChanged(int,int)));
    QObject::connect(model, SIGNAL(modelReset(int,int)), q, SLOT(_q_modelReset(int,int)));
}

void QQuickVisualDataModelPrivate::init()
{
    Q_Q(QQuickVisualDataModel);
    m_compositor.setRemoveGroups(Compositor::GroupMask & ~Compositor::PersistedFlag);

    m_adaptorModel = new QQuickVisualAdaptorModel;
    QObject::connect(m_adaptorModel, SIGNAL(rootIndexChanged()), q, SIGNAL(rootIndexChanged()));

    m_items = new QQuickVisualDataGroup(QStringLiteral("items"), q, Compositor::Default, q);
    m_items->setDefaultInclude(true);
    m_persistedItems = new QQuickVisualDataGroup(QStringLiteral("persistedItems"), q, Compositor::Persisted, q);
    QQuickVisualDataGroupPrivate::get(m_items)->emitters.insert(this);
}

QQuickVisualDataModel::QQuickVisualDataModel()
: QQuickVisualModel(*(new QQuickVisualDataModelPrivate(0)))
{
    Q_D(QQuickVisualDataModel);
    d->init();
}

QQuickVisualDataModel::QQuickVisualDataModel(QDeclarativeContext *ctxt, QObject *parent)
: QQuickVisualModel(*(new QQuickVisualDataModelPrivate(ctxt)), parent)
{
    Q_D(QQuickVisualDataModel);
    d->init();
}

QQuickVisualDataModel::~QQuickVisualDataModel()
{
    Q_D(QQuickVisualDataModel);
    foreach (QQuickVisualDataModelCacheItem *cacheItem, d->m_cache) {
        cacheItem->object = 0;
        cacheItem->objectRef = 0;
        if (!cacheItem->isReferenced())
            delete cacheItem;
    }

    delete d->m_adaptorModel;
    if (d->m_cacheMetaType)
        d->m_cacheMetaType->release();
}


void QQuickVisualDataModel::classBegin()
{
}

void QQuickVisualDataModel::componentComplete()
{
    Q_D(QQuickVisualDataModel);
    d->m_complete = true;

    int defaultGroups = 0;
    QStringList groupNames;
    groupNames.append(QStringLiteral("items"));
    groupNames.append(QStringLiteral("persistedItems"));
    if (QQuickVisualDataGroupPrivate::get(d->m_items)->defaultInclude)
        defaultGroups |= Compositor::DefaultFlag;
    if (QQuickVisualDataGroupPrivate::get(d->m_persistedItems)->defaultInclude)
        defaultGroups |= Compositor::PersistedFlag;
    for (int i = 3; i < d->m_groupCount; ++i) {
        QString name = d->m_groups[i]->name();
        if (name.isEmpty()) {
            d->m_groups[i] = d->m_groups[d->m_groupCount - 1];
            --d->m_groupCount;
            --i;
        } else if (name.at(0).isUpper()) {
            qmlInfo(d->m_groups[i]) << QQuickVisualDataGroup::tr("Group names must start with a lower case letter");
            d->m_groups[i] = d->m_groups[d->m_groupCount - 1];
            --d->m_groupCount;
            --i;
        } else {
            groupNames.append(name);

            QQuickVisualDataGroupPrivate *group = QQuickVisualDataGroupPrivate::get(d->m_groups[i]);
            group->setModel(this, Compositor::Group(i));
            if (group->defaultInclude)
                defaultGroups |= (1 << i);
        }
    }
    if (!d->m_context)
        d->m_context = qmlContext(this);

    d->m_cacheMetaType = new QQuickVisualDataModelCacheMetaType(
            QDeclarativeEnginePrivate::getV8Engine(d->m_context->engine()), this, groupNames);

    d->m_compositor.setGroupCount(d->m_groupCount);
    d->m_compositor.setDefaultGroups(defaultGroups);
    d->updateFilterGroup();

    while (!d->m_pendingParts.isEmpty())
        static_cast<QQuickVisualPartsModel *>(d->m_pendingParts.first())->updateFilterGroup();

    d->connectModel(d->m_adaptorModel);
    QVector<Compositor::Insert> inserts;
    d->m_reset = true;
    d->m_compositor.append(
            d->m_adaptorModel,
            0,
            qMax(0, d->m_adaptorModel->count()),
            defaultGroups | Compositor::AppendFlag | Compositor::PrependFlag,
            &inserts);
    d->itemsInserted(inserts);
    d->emitChanges();

    if (d->m_adaptorModel->canFetchMore())
        QCoreApplication::postEvent(this, new QEvent(QEvent::UpdateRequest));
}

/*!
    \qmlproperty model QtQuick2::VisualDataModel::model
    This property holds the model providing data for the VisualDataModel.

    The model provides a set of data that is used to create the items
    for a view.  For large or dynamic datasets the model is usually
    provided by a C++ model object.  The C++ model object must be a \l
    {QAbstractItemModel} subclass or a simple list.

    Models can also be created directly in QML, using a \l{ListModel} or
    \l{XmlListModel}.

    \sa {qmlmodels}{Data Models}
*/
QVariant QQuickVisualDataModel::model() const
{
    Q_D(const QQuickVisualDataModel);
    return d->m_adaptorModel->model();
}

void QQuickVisualDataModel::setModel(const QVariant &model)
{
    Q_D(QQuickVisualDataModel);
    d->m_adaptorModel->setModel(model, d->m_context ? d->m_context->engine() : qmlEngine(this));
    if (d->m_complete && d->m_adaptorModel->canFetchMore())
        QCoreApplication::postEvent(this, new QEvent(QEvent::UpdateRequest));
}

/*!
    \qmlproperty Component QtQuick2::VisualDataModel::delegate

    The delegate provides a template defining each item instantiated by a view.
    The index is exposed as an accessible \c index property.  Properties of the
    model are also available depending upon the type of \l {qmlmodels}{Data Model}.
*/
QDeclarativeComponent *QQuickVisualDataModel::delegate() const
{
    Q_D(const QQuickVisualDataModel);
    return d->m_delegate;
}

void QQuickVisualDataModel::setDelegate(QDeclarativeComponent *delegate)
{
    Q_D(QQuickVisualDataModel);
    if (d->m_transaction) {
        qmlInfo(this) << tr("The delegate of a VisualDataModel cannot be changed within onUpdated.");
        return;
    }
    bool wasValid = d->m_delegate != 0;
    d->m_delegate = delegate;
    d->m_delegateValidated = false;
    if (wasValid && d->m_complete) {
        for (int i = 1; i < d->m_groupCount; ++i) {
            QQuickVisualDataGroupPrivate::get(d->m_groups[i])->changeSet.remove(
                    0, d->m_compositor.count(Compositor::Group(i)));
        }
    }
    if (d->m_complete && d->m_delegate) {
        for (int i = 1; i < d->m_groupCount; ++i) {
            QQuickVisualDataGroupPrivate::get(d->m_groups[i])->changeSet.insert(
                    0, d->m_compositor.count(Compositor::Group(i)));
        }
    }
    d->emitChanges();
}

/*!
    \qmlproperty QModelIndex QtQuick2::VisualDataModel::rootIndex

    QAbstractItemModel provides a hierarchical tree of data, whereas
    QML only operates on list data.  \c rootIndex allows the children of
    any node in a QAbstractItemModel to be provided by this model.

    This property only affects models of type QAbstractItemModel that
    are hierarchical (e.g, a tree model).

    For example, here is a simple interactive file system browser.
    When a directory name is clicked, the view's \c rootIndex is set to the
    QModelIndex node of the clicked directory, thus updating the view to show
    the new directory's contents.

    \c main.cpp:
    \snippet doc/src/snippets/declarative/visualdatamodel_rootindex/main.cpp 0

    \c view.qml:
    \snippet doc/src/snippets/declarative/visualdatamodel_rootindex/view.qml 0

    If the \l model is a QAbstractItemModel subclass, the delegate can also
    reference a \c hasModelChildren property (optionally qualified by a
    \e model. prefix) that indicates whether the delegate's model item has
    any child nodes.


    \sa modelIndex(), parentModelIndex()
*/
QVariant QQuickVisualDataModel::rootIndex() const
{
    Q_D(const QQuickVisualDataModel);
    return d->m_adaptorModel->rootIndex();
}

void QQuickVisualDataModel::setRootIndex(const QVariant &root)
{
    Q_D(QQuickVisualDataModel);
    d->m_adaptorModel->setRootIndex(root);
}

/*!
    \qmlmethod QModelIndex QtQuick2::VisualDataModel::modelIndex(int index)

    QAbstractItemModel provides a hierarchical tree of data, whereas
    QML only operates on list data.  This function assists in using
    tree models in QML.

    Returns a QModelIndex for the specified index.
    This value can be assigned to rootIndex.

    \sa rootIndex
*/
QVariant QQuickVisualDataModel::modelIndex(int idx) const
{
    Q_D(const QQuickVisualDataModel);
    return d->m_adaptorModel->modelIndex(idx);
}

/*!
    \qmlmethod QModelIndex QtQuick2::VisualDataModel::parentModelIndex()

    QAbstractItemModel provides a hierarchical tree of data, whereas
    QML only operates on list data.  This function assists in using
    tree models in QML.

    Returns a QModelIndex for the parent of the current rootIndex.
    This value can be assigned to rootIndex.

    \sa rootIndex
*/
QVariant QQuickVisualDataModel::parentModelIndex() const
{
    Q_D(const QQuickVisualDataModel);
    return d->m_adaptorModel->parentModelIndex();
}

/*!
    \qmlproperty int QtQuick2::VisualDataModel::count
*/

int QQuickVisualDataModel::count() const
{
    Q_D(const QQuickVisualDataModel);
    if (!d->m_delegate)
        return 0;
    return d->m_compositor.count(d->m_compositorGroup);
}

void QQuickVisualDataModelPrivate::destroy(QObject *object)
{
    QObjectPrivate *p = QObjectPrivate::get(object);
    Q_ASSERT(p->declarativeData);
    QDeclarativeData *data = static_cast<QDeclarativeData*>(p->declarativeData);
    if (data->ownContext && data->context)
        data->context->clearContext();
    object->deleteLater();
}

QQuickVisualDataModel::ReleaseFlags QQuickVisualDataModelPrivate::release(QObject *object)
{
    QQuickVisualDataModel::ReleaseFlags stat = 0;
    if (!object)
        return stat;

    int cacheIndex = cacheIndexOf(object);
    if (cacheIndex != -1) {
        QQuickVisualDataModelCacheItem *cacheItem = m_cache.at(cacheIndex);
        if (cacheItem->releaseObject()) {
            destroy(object);
            cacheItem->object = 0;
            stat |= QQuickVisualModel::Destroyed;
            if (!cacheItem->isReferenced()) {
                m_compositor.clearFlags(Compositor::Cache, cacheIndex, 1, Compositor::CacheFlag);
                m_cache.removeAt(cacheIndex);
                delete cacheItem;
                Q_ASSERT(m_cache.count() == m_compositor.count(Compositor::Cache));
            }
        } else {
            stat |= QQuickVisualDataModel::Referenced;
        }
    }
    return stat;
}

/*
  Returns ReleaseStatus flags.
*/

QQuickVisualDataModel::ReleaseFlags QQuickVisualDataModel::release(QQuickItem *item)
{
    Q_D(QQuickVisualDataModel);
    QQuickVisualModel::ReleaseFlags stat = d->release(item);
    if (stat & Destroyed)
        item->setParentItem(0);
    return stat;
}

void QQuickVisualDataModelPrivate::group_append(
        QDeclarativeListProperty<QQuickVisualDataGroup> *property, QQuickVisualDataGroup *group)
{
    QQuickVisualDataModelPrivate *d = static_cast<QQuickVisualDataModelPrivate *>(property->data);
    if (d->m_complete)
        return;
    if (d->m_groupCount == 11) {
        qmlInfo(d->q_func()) << QQuickVisualDataModel::tr("The maximum number of supported VisualDataGroups is 8");
        return;
    }
    d->m_groups[d->m_groupCount] = group;
    d->m_groupCount += 1;
}

int QQuickVisualDataModelPrivate::group_count(
        QDeclarativeListProperty<QQuickVisualDataGroup> *property)
{
    QQuickVisualDataModelPrivate *d = static_cast<QQuickVisualDataModelPrivate *>(property->data);
    return d->m_groupCount - 1;
}

QQuickVisualDataGroup *QQuickVisualDataModelPrivate::group_at(
        QDeclarativeListProperty<QQuickVisualDataGroup> *property, int index)
{
    QQuickVisualDataModelPrivate *d = static_cast<QQuickVisualDataModelPrivate *>(property->data);
    return index >= 0 && index < d->m_groupCount - 1
            ? d->m_groups[index - 1]
            : 0;
}

/*!
    \qmlproperty list<VisualDataGroup> QtQuick2::VisualDataModel::groups

    This property holds a visual data model's group definitions.

    Groups define a sub-set of the items in a visual data model and can be used to filter
    a model.

    For every group defined in a VisualDataModel two attached properties are added to each
    delegate item.  The first of the form VisualDataModel.in\e{GroupName} holds whether the
    item belongs to the group and the second VisualDataModel.\e{groupName}Index holds the
    index of the item in that group.

    The following example illustrates using groups to select items in a model.

    \snippet doc/src/snippets/declarative/visualdatagroup.qml 0
*/

QDeclarativeListProperty<QQuickVisualDataGroup> QQuickVisualDataModel::groups()
{
    Q_D(QQuickVisualDataModel);
    return QDeclarativeListProperty<QQuickVisualDataGroup>(
            this,
            d,
            QQuickVisualDataModelPrivate::group_append,
            QQuickVisualDataModelPrivate::group_count,
            QQuickVisualDataModelPrivate::group_at);
}

/*!
    \qmlproperty VisualDataGroup QtQuick2::VisualDataModel::items

    This property holds visual data model's default group to which all new items are added.
*/

QQuickVisualDataGroup *QQuickVisualDataModel::items()
{
    Q_D(QQuickVisualDataModel);
    return d->m_items;
}

/*!
    \qmlproperty VisualDataGroup QtQuick2::VisualDataModel::persistedItems

    This property holds visual data model's persisted items group.

    Items in this group are not destroyed when released by a view, instead they are persisted
    until removed from the group.

    An item can be removed from the persistedItems group by setting the
    VisualDataModel.inPersistedItems property to false.  If the item is not referenced by a view
    at that time it will be destroyed.  Adding an item to this group will not create a new
    instance.

    Items returned by the \l QtQuick2::VisualDataGroup::create() function are automatically added
    to this group.
*/

QQuickVisualDataGroup *QQuickVisualDataModel::persistedItems()
{
    Q_D(QQuickVisualDataModel);
    return d->m_persistedItems;
}

/*!
    \qmlproperty string QtQuick2::VisualDataModel::filterOnGroup

    This property holds the name of the group used to filter the visual data model.

    Only items which belong to this group are visible to a view.

    By default this is the \l items group.
*/

QString QQuickVisualDataModel::filterGroup() const
{
    Q_D(const QQuickVisualDataModel);
    return d->m_filterGroup;
}

void QQuickVisualDataModel::setFilterGroup(const QString &group)
{
    Q_D(QQuickVisualDataModel);

    if (d->m_transaction) {
        qmlInfo(this) << tr("The group of a VisualDataModel cannot be changed within onChanged");
        return;
    }

    if (d->m_filterGroup != group) {
        d->m_filterGroup = group;
        d->updateFilterGroup();
        emit filterGroupChanged();
    }
}

void QQuickVisualDataModel::resetFilterGroup()
{
    setFilterGroup(QStringLiteral("items"));
}

void QQuickVisualDataModelPrivate::updateFilterGroup()
{
    Q_Q(QQuickVisualDataModel);
    if (!m_cacheMetaType)
        return;

    QDeclarativeListCompositor::Group previousGroup = m_compositorGroup;
    m_compositorGroup = Compositor::Default;
    for (int i = 1; i < m_groupCount; ++i) {
        if (m_filterGroup == m_cacheMetaType->groupNames.at(i - 1)) {
            m_compositorGroup = Compositor::Group(i);
            break;
        }
    }

    QQuickVisualDataGroupPrivate::get(m_groups[m_compositorGroup])->emitters.insert(this);
    if (m_compositorGroup != previousGroup) {
        QVector<QDeclarativeChangeSet::Remove> removes;
        QVector<QDeclarativeChangeSet::Insert> inserts;
        m_compositor.transition(previousGroup, m_compositorGroup, &removes, &inserts);

        QDeclarativeChangeSet changeSet;
        changeSet.apply(removes, inserts);
        emit q->modelUpdated(changeSet, false);

        if (changeSet.difference() != 0)
            emit q->countChanged();

        if (m_parts) {
            foreach (QQuickVisualPartsModel *model, m_parts->models)
                model->updateFilterGroup(m_compositorGroup, changeSet);
        }
    }
}

/*!
    \qmlproperty object QtQuick2::VisualDataModel::parts

    The \a parts property selects a VisualDataModel which creates
    delegates from the part named.  This is used in conjunction with
    the \l Package element.

    For example, the code below selects a model which creates
    delegates named \e list from a \l Package:

    \code
    VisualDataModel {
        id: visualModel
        delegate: Package {
            Item { Package.name: "list" }
        }
        model: myModel
    }

    ListView {
        width: 200; height:200
        model: visualModel.parts.list
    }
    \endcode

    \sa Package
*/

QObject *QQuickVisualDataModel::parts()
{
    Q_D(QQuickVisualDataModel);
    if (!d->m_parts)
        d->m_parts = new QQuickVisualDataModelParts(this);
    return d->m_parts;
}

void QQuickVisualDataModelPrivate::emitCreatedPackage(Compositor::iterator at, QDeclarativePackage *package)
{
    for (int i = 1; i < m_groupCount; ++i)
        QQuickVisualDataGroupPrivate::get(m_groups[i])->createdPackage(at.index[i], package);
}

void QQuickVisualDataModelPrivate::emitDestroyingPackage(QDeclarativePackage *package)
{
    for (int i = 1; i < m_groupCount; ++i)
        QQuickVisualDataGroupPrivate::get(m_groups[i])->destroyingPackage(package);
}

QObject *QQuickVisualDataModelPrivate::object(Compositor::Group group, int index, bool complete, bool reference)
{
    Q_Q(QQuickVisualDataModel);

    Compositor::iterator it = m_compositor.find(group, index);
    QQuickVisualDataModelCacheItem *cacheItem = it->inCache() ? m_cache.at(it.cacheIndex) : 0;

    if (!cacheItem) {
        cacheItem = new QQuickVisualDataModelCacheItem(m_cacheMetaType);
        for (int i = 0; i < m_groupCount; ++i)
            cacheItem->index[i] = it.index[i];
        cacheItem->groups = it->flags & Compositor::GroupMask;
    }

    if (!cacheItem->object) {
        QObject *data = m_adaptorModel->data(it.modelIndex());

        QDeclarativeContext *creationContext = m_delegate->creationContext();
        QDeclarativeContext *rootContext = new QDeclarativeContext(
                creationContext ? creationContext : m_context.data());
        QDeclarativeContext *ctxt = rootContext;
        if (m_adaptorModel->flags() & QQuickVisualAdaptorModel::ProxiedObject) {
            if (QQuickVisualAdaptorModelProxyInterface *proxy = qobject_cast<QQuickVisualAdaptorModelProxyInterface *>(data)) {
                ctxt->setContextObject(proxy->proxiedObject());
                ctxt = new QDeclarativeContext(ctxt, ctxt);
            }
        }

        QDeclarative_setParent_noEvent(data, ctxt);
        ctxt->setContextProperty(QLatin1String("model"), data);
        ctxt->setContextObject(data);

        m_completePending = false;
        cacheItem->object = m_delegate->beginCreate(ctxt);

        if (cacheItem->object) {
            QDeclarative_setParent_noEvent(rootContext, cacheItem->object);
            if (!it->inCache()) {
                m_cache.insert(it.cacheIndex, cacheItem);
                m_compositor.setFlags(it, 1, Compositor::CacheFlag);
                Q_ASSERT(m_cache.count() == m_compositor.count(Compositor::Cache));
            }

            cacheItem->attached = QQuickVisualDataModelAttached::properties(cacheItem->object);
            cacheItem->attached->m_cacheItem = cacheItem;
            new QQuickVisualDataModelAttachedMetaObject(cacheItem->attached, m_cacheMetaType);
            cacheItem->attached->emitChanges();

            if (QDeclarativePackage *package = qobject_cast<QDeclarativePackage *>(cacheItem->object)) {
                emitCreatedPackage(it, package);
            } else if (!reference) {
                if (QQuickItem *item = qobject_cast<QQuickItem *>(cacheItem->object))
                    emitCreatedItem(it, item);
            }

            m_completePending = !complete;
            if (complete)
                m_delegate->completeCreate();
        } else {
            delete rootContext;
            if (!it->inCache())
                delete cacheItem;
            qmlInfo(q, m_delegate->errors()) << "Error creating delegate";
            return 0;
        }
    }

    if (index == m_compositor.count(group) - 1 && m_adaptorModel->canFetchMore())
        QCoreApplication::postEvent(q, new QEvent(QEvent::UpdateRequest));
    if (reference)
        cacheItem->referenceObject();
    return cacheItem->object;
}

QQuickItem *QQuickVisualDataModel::item(int index, bool complete)
{
    Q_D(QQuickVisualDataModel);
    if (!d->m_delegate || index < 0 || index >= d->m_compositor.count(d->m_compositorGroup)) {
        qWarning() << "VisualDataModel::item: index out range" << index << d->m_compositor.count(d->m_compositorGroup);
        return 0;
    }

    QObject *object = d->object(d->m_compositorGroup, index, complete, true);
    if (QQuickItem *item = qobject_cast<QQuickItem *>(object))
        return item;
    if (d->m_completePending)
        completeItem();
    d->release(object);
    if (!d->m_delegateValidated) {
        if (object)
            qmlInfo(d->m_delegate) << QQuickVisualDataModel::tr("Delegate component must be Item type.");
        d->m_delegateValidated = true;
    }
    return 0;
}

bool QQuickVisualDataModel::completePending() const
{
    Q_D(const QQuickVisualDataModel);
    return d->m_completePending;
}

void QQuickVisualDataModel::completeItem()
{
    Q_D(QQuickVisualDataModel);
    d->m_delegate->completeCreate();
    d->m_completePending = false;
}

QString QQuickVisualDataModelPrivate::stringValue(Compositor::Group group, int index, const QString &name)
{
    Compositor::iterator it = m_compositor.find(group, index);
    if (QQuickVisualAdaptorModel *model = it.list<QQuickVisualAdaptorModel>()) {
        return model->stringValue(it.modelIndex(), name);
    }
    return QString();
}

QString QQuickVisualDataModel::stringValue(int index, const QString &name)
{
    Q_D(QQuickVisualDataModel);
    return d->stringValue(d->m_compositorGroup, index, name);
}

int QQuickVisualDataModelPrivate::cacheIndexOf(QObject *object) const
{
    for (int cacheIndex = 0; cacheIndex < m_cache.count(); ++cacheIndex) {
        if (m_cache.at(cacheIndex)->object == object)
            return cacheIndex;
    }
    return -1;
}

int QQuickVisualDataModel::indexOf(QQuickItem *item, QObject *) const
{
    Q_D(const QQuickVisualDataModel);
    const int cacheIndex = d->cacheIndexOf(item);
    return cacheIndex != -1
            ? d->m_cache.at(cacheIndex)->index[d->m_compositorGroup]
            : -1;
}

void QQuickVisualDataModel::setWatchedRoles(QList<QByteArray> roles)
{
    Q_D(QQuickVisualDataModel);
    d->m_adaptorModel->replaceWatchedRoles(d->watchedRoles, roles);
    d->watchedRoles = roles;
}

void QQuickVisualDataModelPrivate::addGroups(Compositor::Group group, int index, int count, int groupFlags)
{
    QVector<Compositor::Insert> inserts;
    m_compositor.setFlags(group, index, count, groupFlags, &inserts);
    itemsInserted(inserts);
    emitChanges();
}

void QQuickVisualDataModelPrivate::removeGroups(Compositor::Group group, int index, int count, int groupFlags)
{
    QVector<Compositor::Remove> removes;
    m_compositor.clearFlags(group, index, count, groupFlags, &removes);
    itemsRemoved(removes);
    emitChanges();
}

void QQuickVisualDataModelPrivate::setGroups(Compositor::Group group, int index, int count, int groupFlags)
{
    QVector<Compositor::Insert> inserts;
    m_compositor.setFlags(group, index, count, groupFlags, &inserts);
    itemsInserted(inserts);

    const int removeFlags = ~groupFlags & Compositor::GroupMask;
    QVector<Compositor::Remove> removes;
    m_compositor.clearFlags(group, index, count, removeFlags, &removes);
    itemsRemoved(removes);

    emitChanges();
}

bool QQuickVisualDataModel::event(QEvent *e)
{
    Q_D(QQuickVisualDataModel);
    if (e->type() == QEvent::UpdateRequest)
        d->m_adaptorModel->fetchMore();
    return QQuickVisualModel::event(e);
}

void QQuickVisualDataModelPrivate::itemsChanged(const QVector<Compositor::Change> &changes)
{
    if (!m_delegate)
        return;

    QVarLengthArray<QVector<QDeclarativeChangeSet::Change>, Compositor::MaximumGroupCount> translatedChanges(m_groupCount);

    foreach (const Compositor::Change &change, changes) {
        for (int i = 1; i < m_groupCount; ++i) {
            if (change.inGroup(i)) {
                translatedChanges[i].append(
                        QDeclarativeChangeSet::Change(change.index[i], change.count));
            }
        }
    }

    for (int i = 1; i < m_groupCount; ++i)
        QQuickVisualDataGroupPrivate::get(m_groups[i])->changeSet.apply(translatedChanges.at(i));
}

void QQuickVisualDataModel::_q_itemsChanged(int index, int count)
{
    Q_D(QQuickVisualDataModel);
    if (count <= 0)
        return;
    QVector<Compositor::Change> changes;
    d->m_compositor.listItemsChanged(d->m_adaptorModel, index, count, &changes);
    d->itemsChanged(changes);
    d->emitChanges();
}

void QQuickVisualDataModelPrivate::itemsInserted(
        const QVector<Compositor::Insert> &inserts,
        QVarLengthArray<QVector<QDeclarativeChangeSet::Insert>, Compositor::MaximumGroupCount> *translatedInserts,
        QHash<int, QList<QQuickVisualDataModelCacheItem *> > *movedItems)
{
    int cacheIndex = 0;

    int inserted[Compositor::MaximumGroupCount];
    for (int i = 1; i < m_groupCount; ++i)
        inserted[i] = 0;

    foreach (const Compositor::Insert &insert, inserts) {
        for (; cacheIndex < insert.cacheIndex; ++cacheIndex) {
            QQuickVisualDataModelCacheItem *cacheItem = m_cache.at(cacheIndex);
            if (!cacheItem->groups)
                continue;
            for (int i = 1; i < m_groupCount; ++i)
                cacheItem->index[i] += inserted[i];
        }
        for (int i = 1; i < m_groupCount; ++i) {
            if (insert.inGroup(i)) {
                (*translatedInserts)[i].append(
                        QDeclarativeChangeSet::Insert(insert.index[i], insert.count, insert.moveId));
                inserted[i] += insert.count;
            }
        }

        if (!insert.inCache())
            continue;

        if (movedItems && insert.isMove()) {
            QList<QQuickVisualDataModelCacheItem *> items = movedItems->take(insert.moveId);
            Q_ASSERT(items.count() == insert.count);
            m_cache = m_cache.mid(0, insert.cacheIndex) + items + m_cache.mid(insert.cacheIndex);
        }
        if (insert.inGroup()) {
            for (int offset = 0; cacheIndex < insert.cacheIndex + insert.count; ++cacheIndex, ++offset) {
                QQuickVisualDataModelCacheItem *cacheItem = m_cache.at(cacheIndex);
                cacheItem->groups |= insert.flags & Compositor::GroupMask;
                for (int i = 1; i < m_groupCount; ++i) {
                    cacheItem->index[i] = cacheItem->groups & (1 << i)
                            ? insert.index[i] + offset
                            : insert.index[i];
                }
            }
        } else {
            cacheIndex = insert.cacheIndex + insert.count;
        }
    }
    for (; cacheIndex < m_cache.count(); ++cacheIndex) {
        QQuickVisualDataModelCacheItem *cacheItem = m_cache.at(cacheIndex);
        if (!cacheItem->groups)
            continue;
        for (int i = 1; i < m_groupCount; ++i)
            cacheItem->index[i] += inserted[i];
    }
}

void QQuickVisualDataModelPrivate::itemsInserted(const QVector<Compositor::Insert> &inserts)
{
    QVarLengthArray<QVector<QDeclarativeChangeSet::Insert>, Compositor::MaximumGroupCount> translatedInserts(m_groupCount);
    itemsInserted(inserts, &translatedInserts);
    Q_ASSERT(m_cache.count() == m_compositor.count(Compositor::Cache));
    if (!m_delegate)
        return;

    for (int i = 1; i < m_groupCount; ++i)
        QQuickVisualDataGroupPrivate::get(m_groups[i])->changeSet.apply(translatedInserts.at(i));
}

void QQuickVisualDataModel::_q_itemsInserted(int index, int count)
{

    Q_D(QQuickVisualDataModel);
    if (count <= 0)
        return;
    QVector<Compositor::Insert> inserts;
    d->m_compositor.listItemsInserted(d->m_adaptorModel, index, count, &inserts);
    d->itemsInserted(inserts);
    d->emitChanges();
}

void QQuickVisualDataModelPrivate::itemsRemoved(
        const QVector<Compositor::Remove> &removes,
        QVarLengthArray<QVector<QDeclarativeChangeSet::Remove>, Compositor::MaximumGroupCount> *translatedRemoves,
        QHash<int, QList<QQuickVisualDataModelCacheItem *> > *movedItems)
{
    int cacheIndex = 0;
    int removedCache = 0;

    int removed[Compositor::MaximumGroupCount];
    for (int i = 1; i < m_groupCount; ++i)
        removed[i] = 0;

    foreach (const Compositor::Remove &remove, removes) {
        for (; cacheIndex < remove.cacheIndex; ++cacheIndex) {
            QQuickVisualDataModelCacheItem *cacheItem = m_cache.at(cacheIndex);
            if (!cacheItem->groups)
                continue;
            for (int i = 1; i < m_groupCount; ++i)
                cacheItem->index[i] -= removed[i];
        }
        for (int i = 1; i < m_groupCount; ++i) {
            if (remove.inGroup(i)) {
                (*translatedRemoves)[i].append(
                        QDeclarativeChangeSet::Remove(remove.index[i], remove.count, remove.moveId));
                removed[i] += remove.count;
            }
        }

        if (!remove.inCache())
            continue;

        if (movedItems && remove.isMove()) {
            movedItems->insert(remove.moveId, m_cache.mid(remove.cacheIndex, remove.count));
            QList<QQuickVisualDataModelCacheItem *>::iterator begin = m_cache.begin() + remove.cacheIndex;
            QList<QQuickVisualDataModelCacheItem *>::iterator end = begin + remove.count;
            m_cache.erase(begin, end);
        } else {
            for (; cacheIndex < remove.cacheIndex + remove.count - removedCache; ++cacheIndex) {
                QQuickVisualDataModelCacheItem *cacheItem = m_cache.at(cacheIndex);
                if (remove.inGroup(Compositor::Persisted) && cacheItem->objectRef == 0) {
                    destroy(cacheItem->object);
                    if (QDeclarativePackage *package = qobject_cast<QDeclarativePackage *>(cacheItem->object))
                        emitDestroyingPackage(package);
                    else if (QQuickItem *item = qobject_cast<QQuickItem *>(cacheItem->object))
                        emitDestroyingItem(item);
                    cacheItem->object = 0;
                }
                if (remove.groups() == cacheItem->groups && !cacheItem->isReferenced()) {
                    m_compositor.clearFlags(Compositor::Cache, cacheIndex, 1, Compositor::CacheFlag);
                    m_cache.removeAt(cacheIndex);
                    delete cacheItem;
                    --cacheIndex;
                    ++removedCache;
                    Q_ASSERT(m_cache.count() == m_compositor.count(Compositor::Cache));
                } else if (remove.groups() == cacheItem->groups) {
                    cacheItem->groups = 0;
                    for (int i = 1; i < m_groupCount; ++i)
                        cacheItem->index[i] = -1;
                } else {
                    for (int i = 1; i < m_groupCount; ++i) {
                        if (remove.inGroup(i))
                            cacheItem->index[i] = remove.index[i];
                    }
                    cacheItem->groups &= ~remove.flags & Compositor::GroupMask;
                }
            }
        }
    }

    for (; cacheIndex < m_cache.count(); ++cacheIndex) {
        QQuickVisualDataModelCacheItem *cacheItem = m_cache.at(cacheIndex);
        if (!cacheItem->groups)
            continue;
        for (int i = 1; i < m_groupCount; ++i)
            cacheItem->index[i] -= removed[i];
    }
}

void QQuickVisualDataModelPrivate::itemsRemoved(const QVector<Compositor::Remove> &removes)
{
    QVarLengthArray<QVector<QDeclarativeChangeSet::Remove>, Compositor::MaximumGroupCount> translatedRemoves(m_groupCount);
    itemsRemoved(removes, &translatedRemoves);
    Q_ASSERT(m_cache.count() == m_compositor.count(Compositor::Cache));
    if (!m_delegate)
        return;

    for (int i = 1; i < m_groupCount; ++i)
       QQuickVisualDataGroupPrivate::get(m_groups[i])->changeSet.apply(translatedRemoves.at(i));
}

void QQuickVisualDataModel::_q_itemsRemoved(int index, int count)
{
    Q_D(QQuickVisualDataModel);
    if (count <= 0)
        return;

    QVector<Compositor::Remove> removes;
    d->m_compositor.listItemsRemoved(d->m_adaptorModel, index, count, &removes);
    d->itemsRemoved(removes);
    d->emitChanges();
}

void QQuickVisualDataModelPrivate::itemsMoved(
        const QVector<Compositor::Remove> &removes, const QVector<Compositor::Insert> &inserts)
{
    QHash<int, QList<QQuickVisualDataModelCacheItem *> > movedItems;

    QVarLengthArray<QVector<QDeclarativeChangeSet::Remove>, Compositor::MaximumGroupCount> translatedRemoves(m_groupCount);
    itemsRemoved(removes, &translatedRemoves, &movedItems);

    QVarLengthArray<QVector<QDeclarativeChangeSet::Insert>, Compositor::MaximumGroupCount> translatedInserts(m_groupCount);
    itemsInserted(inserts, &translatedInserts, &movedItems);
    Q_ASSERT(m_cache.count() == m_compositor.count(Compositor::Cache));
    Q_ASSERT(movedItems.isEmpty());
    if (!m_delegate)
        return;

    for (int i = 1; i < m_groupCount; ++i) {
        QQuickVisualDataGroupPrivate::get(m_groups[i])->changeSet.apply(
                    translatedRemoves.at(i),
                    translatedInserts.at(i));
    }
}

void QQuickVisualDataModel::_q_itemsMoved(int from, int to, int count)
{
    Q_D(QQuickVisualDataModel);
    if (count <= 0)
        return;

    QVector<Compositor::Remove> removes;
    QVector<Compositor::Insert> inserts;
    d->m_compositor.listItemsMoved(d->m_adaptorModel, from, to, count, &removes, &inserts);
    d->itemsMoved(removes, inserts);
    d->emitChanges();
}

template <typename T> v8::Local<v8::Array>
QQuickVisualDataModelPrivate::buildChangeList(const QVector<T> &changes)
{
    v8::Local<v8::Array> indexes = v8::Array::New(changes.count());
    v8::Local<v8::String> indexKey = v8::String::New("index");
    v8::Local<v8::String> countKey = v8::String::New("count");
    v8::Local<v8::String> moveIdKey = v8::String::New("moveId");

    for (int i = 0; i < changes.count(); ++i) {
        v8::Local<v8::Object> object = v8::Object::New();
        object->Set(indexKey, v8::Integer::New(changes.at(i).index));
        object->Set(countKey, v8::Integer::New(changes.at(i).count));
        object->Set(moveIdKey, changes.at(i).moveId != -1 ? v8::Integer::New(changes.at(i).count) : v8::Undefined());
        indexes->Set(i, object);
    }
    return indexes;
}

void QQuickVisualDataModelPrivate::emitModelUpdated(const QDeclarativeChangeSet &changeSet, bool reset)
{
    Q_Q(QQuickVisualDataModel);
    emit q->modelUpdated(changeSet, reset);
    if (changeSet.difference() != 0)
        emit q->countChanged();
}

void QQuickVisualDataModelPrivate::emitChanges()
{
    if (m_transaction || !m_complete)
        return;

    m_transaction = true;
    QV8Engine *engine = QDeclarativeEnginePrivate::getV8Engine(m_context->engine());
    for (int i = 1; i < m_groupCount; ++i)
        QQuickVisualDataGroupPrivate::get(m_groups[i])->emitChanges(engine);
    m_transaction = false;

    const bool reset = m_reset;
    m_reset = false;
    for (int i = 1; i < m_groupCount; ++i)
        QQuickVisualDataGroupPrivate::get(m_groups[i])->emitModelUpdated(reset);

    foreach (QQuickVisualDataModelCacheItem *cacheItem, m_cache) {
        if (cacheItem->object)
            cacheItem->attached->emitChanges();
    }
}

void QQuickVisualDataModel::_q_modelReset(int oldCount, int newCount)
{
    Q_D(QQuickVisualDataModel);
    if (!d->m_delegate)
        return;

    QVector<Compositor::Remove> removes;
    QVector<Compositor::Insert> inserts;
    if (oldCount)
        d->m_compositor.listItemsRemoved(d->m_adaptorModel, 0, oldCount, &removes);
    if (newCount)
        d->m_compositor.listItemsInserted(d->m_adaptorModel, 0, newCount, &inserts);
    d->itemsMoved(removes, inserts);
    d->m_reset = true;
    d->emitChanges();
}

QQuickVisualDataModelAttached *QQuickVisualDataModel::qmlAttachedProperties(QObject *obj)
{
    return QQuickVisualDataModelAttached::properties(obj);
}

//============================================================================

QQuickVisualDataModelCacheMetaType::QQuickVisualDataModelCacheMetaType(
        QV8Engine *engine, QQuickVisualDataModel *model, const QStringList &groupNames)
    : model(model)
    , groupCount(groupNames.count() + 1)
    , memberPropertyOffset(QQuickVisualDataModelAttached::staticMetaObject.propertyCount())
    , indexPropertyOffset(QQuickVisualDataModelAttached::staticMetaObject.propertyCount() + groupNames.count())
    , v8Engine(engine)
    , metaObject(0)
    , groupNames(groupNames)
{
    QMetaObjectBuilder builder;
    builder.setFlags(QMetaObjectBuilder::DynamicMetaObject);
    builder.setClassName(QQuickVisualDataModelAttached::staticMetaObject.className());
    builder.setSuperClass(&QQuickVisualDataModelAttached::staticMetaObject);

    v8::HandleScope handleScope;
    v8::Context::Scope contextScope(engine->context());
    v8::Local<v8::FunctionTemplate> ft = v8::FunctionTemplate::New();
    ft->InstanceTemplate()->SetHasExternalResource(true);
    ft->PrototypeTemplate()->SetAccessor(v8::String::New("model"), get_model);
    ft->PrototypeTemplate()->SetAccessor(v8::String::New("groups"), get_groups, set_groups);

    int notifierId = 0;
    for (int i = 0; i < groupNames.count(); ++i, ++notifierId) {
        QString propertyName = QStringLiteral("in") + groupNames.at(i);
        propertyName.replace(2, 1, propertyName.at(2).toUpper());
        builder.addSignal("__" + propertyName.toUtf8() + "Changed()");
        QMetaPropertyBuilder propertyBuilder = builder.addProperty(
                propertyName.toUtf8(), "bool", notifierId);
        propertyBuilder.setWritable(true);

        ft->PrototypeTemplate()->SetAccessor(
                engine->toString(propertyName), get_member, set_member, v8::Int32::New(i + 1));
    }
    for (int i = 0; i < groupNames.count(); ++i, ++notifierId) {
        const QString propertyName = groupNames.at(i) + QStringLiteral("Index");
        builder.addSignal("__" + propertyName.toUtf8() + "Changed()");
        QMetaPropertyBuilder propertyBuilder = builder.addProperty(
                propertyName.toUtf8(), "int", notifierId);
        propertyBuilder.setWritable(true);

        ft->PrototypeTemplate()->SetAccessor(
                engine->toString(propertyName), get_index, 0, v8::Int32::New(i + 1));
    }

    metaObject = builder.toMetaObject();

    constructor = qPersistentNew<v8::Function>(ft->GetFunction());
}

QQuickVisualDataModelCacheMetaType::~QQuickVisualDataModelCacheMetaType()
{
    qFree(metaObject);
    qPersistentDispose(constructor);
}

int QQuickVisualDataModelCacheMetaType::parseGroups(const QStringList &groups) const
{
    int groupFlags = 0;
    foreach (const QString &groupName, groups) {
        int index = groupNames.indexOf(groupName);
        if (index != -1)
            groupFlags |= 2 << index;
    }
    return groupFlags;
}

int QQuickVisualDataModelCacheMetaType::parseGroups(QV8Engine *engine, const v8::Local<v8::Value> &groups) const
{
    int groupFlags = 0;
    if (groups->IsString()) {
        const QString groupName = engine->toString(groups);
        int index = groupNames.indexOf(groupName);
        if (index != -1)
            groupFlags |= 2 << index;
    } else if (groups->IsArray()) {
        v8::Local<v8::Array> array = v8::Local<v8::Array>::Cast(groups);
        for (uint i = 0; i < array->Length(); ++i) {
            const QString groupName = engine->toString(array->Get(i));
            int index = groupNames.indexOf(groupName);
            if (index != -1)
                groupFlags |= 2 << index;
        }
    }
    return groupFlags;
}

v8::Handle<v8::Value> QQuickVisualDataModelCacheMetaType::get_model(
        v8::Local<v8::String>, const v8::AccessorInfo &info)
{
    QQuickVisualDataModelCacheItem *cacheItem = v8_resource_cast<QQuickVisualDataModelCacheItem>(info.This());
    if (!cacheItem)
        V8THROW_ERROR("Not a valid VisualData object");
    if (!cacheItem->metaType->model)
        return v8::Undefined();
    QObject *data = 0;

    QQuickVisualDataModelPrivate *model = QQuickVisualDataModelPrivate::get(cacheItem->metaType->model);
    for (int i = 1; i < cacheItem->metaType->groupCount; ++i) {
        if (cacheItem->groups & (1 << i)) {
            Compositor::iterator it = model->m_compositor.find(
                    Compositor::Group(i), cacheItem->index[i]);
            if (QQuickVisualAdaptorModel *list = it.list<QQuickVisualAdaptorModel>())
                data = list->data(it.modelIndex());
            break;
        }
    }
    if (!data)
        return v8::Undefined();
    return cacheItem->engine->newQObject(data);
}

v8::Handle<v8::Value> QQuickVisualDataModelCacheMetaType::get_groups(
        v8::Local<v8::String>, const v8::AccessorInfo &info)
{
    QQuickVisualDataModelCacheItem *cacheItem = v8_resource_cast<QQuickVisualDataModelCacheItem>(info.This());
    if (!cacheItem)
        V8THROW_ERROR("Not a valid VisualData object");

    QStringList groups;
    for (int i = 1; i < cacheItem->metaType->groupCount; ++i) {
        if (cacheItem->groups & (1 << i))
            groups.append(cacheItem->metaType->groupNames.at(i - 1));
    }

    return cacheItem->engine->fromVariant(groups);
}

void QQuickVisualDataModelCacheMetaType::set_groups(
        v8::Local<v8::String>, v8::Local<v8::Value> value, const v8::AccessorInfo &info)
{
    QQuickVisualDataModelCacheItem *cacheItem = v8_resource_cast<QQuickVisualDataModelCacheItem>(info.This());
    if (!cacheItem)
        V8THROW_ERROR_SETTER("Not a valid VisualData object");

    if (!cacheItem->metaType->model)
        return;
    QQuickVisualDataModelPrivate *model = QQuickVisualDataModelPrivate::get(cacheItem->metaType->model);

    const int groupFlags = model->m_cacheMetaType->parseGroups(cacheItem->engine, value);
    for (int i = 1; i < cacheItem->metaType->groupCount; ++i) {
        if (cacheItem->groups & (1 << i)) {
            model->setGroups(Compositor::Group(i), cacheItem->index[i], 1, groupFlags);
            break;
        }
    }
}

v8::Handle<v8::Value> QQuickVisualDataModelCacheMetaType::get_member(
        v8::Local<v8::String>, const v8::AccessorInfo &info)
{
    QQuickVisualDataModelCacheItem *cacheItem = v8_resource_cast<QQuickVisualDataModelCacheItem>(info.This());
    if (!cacheItem)
        V8THROW_ERROR("Not a valid VisualData object");

    return v8::Boolean::New(cacheItem->groups & (1 << info.Data()->Int32Value()));
}

void QQuickVisualDataModelCacheMetaType::set_member(
        v8::Local<v8::String>, v8::Local<v8::Value> value, const v8::AccessorInfo &info)
{
    QQuickVisualDataModelCacheItem *cacheItem = v8_resource_cast<QQuickVisualDataModelCacheItem>(info.This());
    if (!cacheItem)
        V8THROW_ERROR_SETTER("Not a valid VisualData object");

    if (!cacheItem->metaType->model)
        return;
    QQuickVisualDataModelPrivate *model = QQuickVisualDataModelPrivate::get(cacheItem->metaType->model);

    Compositor::Group group = Compositor::Group(info.Data()->Int32Value());
    const bool member = value->BooleanValue();
    const int groupFlag = (1 << group);
    if (member == ((cacheItem->groups & groupFlag) != 0))
        return;

    for (int i = 1; i < cacheItem->metaType->groupCount; ++i) {
        if (cacheItem->groups & (1 << i)) {
            if (member)
                model->addGroups(Compositor::Group(i), cacheItem->index[i], 1, groupFlag);
            else
                model->removeGroups(Compositor::Group(i), cacheItem->index[i], 1, groupFlag);
            break;
        }
    }
}

v8::Handle<v8::Value> QQuickVisualDataModelCacheMetaType::get_index(
        v8::Local<v8::String>, const v8::AccessorInfo &info)
{
    QQuickVisualDataModelCacheItem *cacheItem = v8_resource_cast<QQuickVisualDataModelCacheItem>(info.This());
    if (!cacheItem)
        V8THROW_ERROR("Not a valid VisualData object");

    return v8::Integer::New(cacheItem->index[info.Data()->Int32Value()]);
}


//---------------------------------------------------------------------------

void QQuickVisualDataModelCacheItem::Dispose()
{
    --scriptRef;
    if (isReferenced())
        return;

    if (metaType->model) {
        QQuickVisualDataModelPrivate *model = QQuickVisualDataModelPrivate::get(metaType->model);
        const int cacheIndex = model->m_cache.indexOf(this);
        if (cacheIndex != -1) {
            model->m_compositor.clearFlags(Compositor::Cache, cacheIndex, 1, Compositor::CacheFlag);
            model->m_cache.removeAt(cacheIndex);
        }
    }
    delete this;
}

//---------------------------------------------------------------------------

QQuickVisualDataModelAttachedMetaObject::QQuickVisualDataModelAttachedMetaObject(
        QQuickVisualDataModelAttached *attached, QQuickVisualDataModelCacheMetaType *metaType)
    : attached(attached)
    , metaType(metaType)
{
    metaType->addref();
    *static_cast<QMetaObject *>(this) = *metaType->metaObject;
    QObjectPrivate::get(attached)->metaObject = this;
}

QQuickVisualDataModelAttachedMetaObject::~QQuickVisualDataModelAttachedMetaObject()
{
    metaType->release();
}

int QQuickVisualDataModelAttachedMetaObject::metaCall(QMetaObject::Call call, int _id, void **arguments)
{
    if (call == QMetaObject::ReadProperty) {
        if (_id >= metaType->indexPropertyOffset) {
            Compositor::Group group = Compositor::Group(_id - metaType->indexPropertyOffset + 1);
            *static_cast<int *>(arguments[0]) = attached->m_cacheItem->index[group];
            return -1;
        } else if (_id >= metaType->memberPropertyOffset) {
            Compositor::Group group = Compositor::Group(_id - metaType->memberPropertyOffset + 1);
            *static_cast<bool *>(arguments[0]) = attached->m_cacheItem->groups & (1 << group);
            return -1;
        }
    } else if (call == QMetaObject::WriteProperty) {
        if (_id >= metaType->memberPropertyOffset) {
            if (!metaType->model)
                return -1;
            Compositor::Group group = Compositor::Group(_id - metaType->memberPropertyOffset + 1);
            const bool member = attached->m_cacheItem->groups & (1 << group);
            if (member != *static_cast<bool *>(arguments[0])) {
                QQuickVisualDataModelPrivate *model = QQuickVisualDataModelPrivate::get(metaType->model);
                const int cacheIndex = model->m_cache.indexOf(attached->m_cacheItem);
                if (member)
                    model->removeGroups(Compositor::Cache, cacheIndex, 1, (1 << group));
                else
                    model->addGroups(Compositor::Cache, cacheIndex, 1, (1 << group));
            }
            return -1;
        }
    }
    return attached->qt_metacall(call, _id, arguments);
}

/*!
    \qmlattachedproperty int QtQuick2::VisualDataModel::model

    This attached property holds the visual data model this delegate instance belongs to.

    It is attached to each instance of the delegate.
*/

QQuickVisualDataModel *QQuickVisualDataModelAttached::model() const
{
    return m_cacheItem ? m_cacheItem->metaType->model : 0;
}

/*!
    \qmlattachedproperty stringlist QtQuick2::VisualDataModel::groups

    This attached property holds the name of VisualDataGroups the item belongs to.

    It is attached to each instance of the delegate.
*/

QStringList QQuickVisualDataModelAttached::groups() const
{
    QStringList groups;

    if (!m_cacheItem)
        return groups;
    for (int i = 1; i < m_cacheItem->metaType->groupCount; ++i) {
        if (m_cacheItem->groups & (1 << i))
            groups.append(m_cacheItem->metaType->groupNames.at(i - 1));
    }
    return groups;
}

void QQuickVisualDataModelAttached::setGroups(const QStringList &groups)
{
    if (!m_cacheItem)
        return;

    QQuickVisualDataModelPrivate *model = QQuickVisualDataModelPrivate::get(m_cacheItem->metaType->model);

    const int cacheIndex = model->m_cache.indexOf(m_cacheItem);
    const int groupFlags = model->m_cacheMetaType->parseGroups(groups);
    model->setGroups(Compositor::Cache, cacheIndex, 1, groupFlags);
}

/*!
    \qmlattachedproperty int QtQuick2::VisualDataModel::inItems

    This attached property holds whether the item belongs to the default \l items VisualDataGroup.

    Changing this property will add or remove the item from the items group.

    It is attached to each instance of the delegate.
*/

/*!
    \qmlattachedproperty int QtQuick2::VisualDataModel::itemsIndex

    This attached property holds the index of the item in the default \l items VisualDataGroup.

    It is attached to each instance of the delegate.
*/

/*!
    \qmlattachedproperty int QtQuick2::VisualDataModel::inPersistedItems

    This attached property holds whether the item belongs to the \l persistedItems VisualDataGroup.

    Changing this property will add or remove the item from the items group.  Change with caution
    as removing an item from the persistedItems group will destroy the current instance if it is
    not referenced by a model.

    It is attached to each instance of the delegate.
*/

/*!
    \qmlattachedproperty int QtQuick2::VisualDataModel::persistedItemsIndex

    This attached property holds the index of the item in the \l persistedItems VisualDataGroup.

    It is attached to each instance of the delegate.
*/

void QQuickVisualDataModelAttached::emitChanges()
{
    if (m_modelChanged) {
        m_modelChanged = false;
        emit modelChanged();
    }

    const int groupChanges = m_previousGroups ^ m_cacheItem->groups;
    m_previousGroups = m_cacheItem->groups;

    int indexChanges = 0;
    for (int i = 1; i < m_cacheItem->metaType->groupCount; ++i) {
        if (m_previousIndex[i] != m_cacheItem->index[i]) {
            m_previousIndex[i] = m_cacheItem->index[i];
            indexChanges |= (1 << i);
        }
    }

    int notifierId = 0;
    const QMetaObject *meta = metaObject();
    for (int i = 1; i < m_cacheItem->metaType->groupCount; ++i, ++notifierId) {
        if (groupChanges & (1 << i))
            QMetaObject::activate(this, meta, notifierId, 0);
    }
    for (int i = 1; i < m_cacheItem->metaType->groupCount; ++i, ++notifierId) {
        if (indexChanges & (1 << i))
            QMetaObject::activate(this, meta, notifierId, 0);
    }

    if (groupChanges)
        emit groupsChanged();
}

//============================================================================

void QQuickVisualDataGroupPrivate::setModel(QQuickVisualDataModel *m, Compositor::Group g)
{
    Q_ASSERT(!model);
    model = m;
    group = g;
}

void QQuickVisualDataGroupPrivate::emitChanges(QV8Engine *engine)
{
    Q_Q(QQuickVisualDataGroup);
    static int idx = signalIndex("changed(QDeclarativeV8Handle,QDeclarativeV8Handle)");
    if (isSignalConnected(idx)) {
        v8::HandleScope handleScope;
        v8::Context::Scope contextScope(engine->context());
        v8::Local<v8::Array> removed  = QQuickVisualDataModelPrivate::buildChangeList(changeSet.removes());
        v8::Local<v8::Array> inserted = QQuickVisualDataModelPrivate::buildChangeList(changeSet.inserts());
        emit q->changed(
                QDeclarativeV8Handle::fromHandle(removed), QDeclarativeV8Handle::fromHandle(inserted));
    }
    if (changeSet.difference() != 0)
        emit q->countChanged();
}

void QQuickVisualDataGroupPrivate::emitModelUpdated(bool reset)
{
    for (QQuickVisualDataGroupEmitterList::iterator it = emitters.begin(); it != emitters.end(); ++it)
        it->emitModelUpdated(changeSet, reset);
    changeSet.clear();
}

void QQuickVisualDataGroupPrivate::createdPackage(int index, QDeclarativePackage *package)
{
    for (QQuickVisualDataGroupEmitterList::iterator it = emitters.begin(); it != emitters.end(); ++it)
        it->createdPackage(index, package);
}

void QQuickVisualDataGroupPrivate::destroyingPackage(QDeclarativePackage *package)
{
    for (QQuickVisualDataGroupEmitterList::iterator it = emitters.begin(); it != emitters.end(); ++it)
        it->destroyingPackage(package);
}

/*!
    \qmlclass VisualDataGroup QQuickVisualDataGroup
    \inqmlmodule QtQuick 2
    \ingroup qml-working-with-data
    \brief The VisualDataGroup encapsulates a filtered set of visual data items.

*/

QQuickVisualDataGroup::QQuickVisualDataGroup(QObject *parent)
    : QObject(*new QQuickVisualDataGroupPrivate, parent)
{
}

QQuickVisualDataGroup::QQuickVisualDataGroup(
        const QString &name, QQuickVisualDataModel *model, int index, QObject *parent)
    : QObject(*new QQuickVisualDataGroupPrivate, parent)
{
    Q_D(QQuickVisualDataGroup);
    d->name = name;
    d->setModel(model, Compositor::Group(index));
}

QQuickVisualDataGroup::~QQuickVisualDataGroup()
{
}

/*!
    \qmlproperty string QtQuick2::VisualDataGroup::name

    This property holds the name of the group.

    Each group in a model must have a unique name starting with a lower case letter.
*/

QString QQuickVisualDataGroup::name() const
{
    Q_D(const QQuickVisualDataGroup);
    return d->name;
}

void QQuickVisualDataGroup::setName(const QString &name)
{
    Q_D(QQuickVisualDataGroup);
    if (d->model)
        return;
    if (d->name != name) {
        d->name = name;
        emit nameChanged();
    }
}

/*!
    \qmlproperty int QtQuick2::VisualDataGroup::count

    This property holds the number of items in the group.
*/

int QQuickVisualDataGroup::count() const
{
    Q_D(const QQuickVisualDataGroup);
    if (!d->model)
        return 0;
    return QQuickVisualDataModelPrivate::get(d->model)->m_compositor.count(d->group);
}

/*!
    \qmlproperty bool QtQuick2::VisualDataGroup::includeByDefault

    This property holds whether new items are assigned to this group by default.
*/

bool QQuickVisualDataGroup::defaultInclude() const
{
    Q_D(const QQuickVisualDataGroup);
    return d->defaultInclude;
}

void QQuickVisualDataGroup::setDefaultInclude(bool include)
{
    Q_D(QQuickVisualDataGroup);
    if (d->defaultInclude != include) {
        d->defaultInclude = include;

        if (d->model) {
            if (include)
                QQuickVisualDataModelPrivate::get(d->model)->m_compositor.setDefaultGroup(d->group);
            else
                QQuickVisualDataModelPrivate::get(d->model)->m_compositor.clearDefaultGroup(d->group);
        }
        emit defaultIncludeChanged();
    }
}

/*!
    \qmlmethod var QtQuick2::VisualDataGroup::get(int index)

    Returns a javascript object describing the item at \a index in the group.

    The returned object contains the same information that is available to a delegate from the
    VisualDataModel attached as well as the model for that item.  It has the properties:

    \list
    \o \b model The model data of the item.  This is the same as the model context property in
    a delegate
    \o \b groups A list the of names of groups the item is a member of.  This property can be
    written to change the item's membership.
    \o \b inItems Whether the item belongs to the \l {QtQuick2::VisualDataModel::items}{items} group.
    Writing to this property will add or remove the item from the group.
    \o \b itemsIndex The index of the item within the \l {QtQuick2::VisualDataModel::items}{items} group.
    \o \b {in\i{GroupName}} Whether the item belongs to the dynamic group \i groupName.  Writing to
    this property will add or remove the item from the group.
    \o \b {\i{groupName}Index} The index of the item within the dynamic group \i groupName.
    \endlist
*/

QDeclarativeV8Handle QQuickVisualDataGroup::get(int index)
{
    Q_D(QQuickVisualDataGroup);
    if (!d->model)
        return QDeclarativeV8Handle::fromHandle(v8::Undefined());;

    QQuickVisualDataModelPrivate *model = QQuickVisualDataModelPrivate::get(d->model);
    if (index < 0 || index >= model->m_compositor.count(d->group)) {
        qmlInfo(this) << tr("get: index out of range");
        return QDeclarativeV8Handle::fromHandle(v8::Undefined());
    }

    Compositor::iterator it = model->m_compositor.find(d->group, index);
    QQuickVisualDataModelCacheItem *cacheItem = it->inCache()
            ? model->m_cache.at(it.cacheIndex)
            : 0;

    if (!cacheItem) {
        cacheItem = new QQuickVisualDataModelCacheItem(model->m_cacheMetaType);
        for (int i = 0; i < model->m_groupCount; ++i)
            cacheItem->index[i] = it.index[i];
        cacheItem->groups = it->flags & Compositor::GroupMask;

        model->m_cache.insert(it.cacheIndex, cacheItem);
        model->m_compositor.setFlags(it, 1, Compositor::CacheFlag);
    }

    ++cacheItem->scriptRef;

    v8::Local<v8::Object> rv = model->m_cacheMetaType->constructor->NewInstance();
    rv->SetExternalResource(cacheItem);
    return QDeclarativeV8Handle::fromHandle(rv);
}

/*!
    \qmlmethod QtQuick2::VisualDataGroup::create(int index)

    Returns a reference to the instantiated item at \a index in the group.

    All items returned by create are added to the persistedItems group.  Items in this
    group remain instantiated when not referenced by any view.
*/

QObject *QQuickVisualDataGroup::create(int index)
{
    Q_D(QQuickVisualDataGroup);
    if (!d->model)
        return 0;

    QQuickVisualDataModelPrivate *model = QQuickVisualDataModelPrivate::get(d->model);
    if (index < 0 || index >= model->m_compositor.count(d->group)) {
        qmlInfo(this) << tr("create: index out of range");
        return 0;
    }

    QObject *object = model->object(d->group, index, true, false);
    if (object)
        model->addGroups(d->group, index, 1, Compositor::PersistedFlag);
    return object;
}

/*!
    \qmlmethod QtQuick2::VisualDataGroup::remove(int index, int count)

    Removes \a count items starting at \a index from the group.
*/

void QQuickVisualDataGroup::remove(QDeclarativeV8Function *args)
{
    Q_D(QQuickVisualDataGroup);
    if (!d->model)
        return;
    int index = -1;
    int count = 1;

    if (args->Length() == 0)
        return;

    int i = 0;
    v8::Local<v8::Value> v = (*args)[i];
    if (!v->IsInt32())
        return;
    index = v->Int32Value();

    if (++i < args->Length()) {
        v = (*args)[i];
        if (v->IsInt32())
            count = v->Int32Value();
    }

    QQuickVisualDataModelPrivate *model = QQuickVisualDataModelPrivate::get(d->model);
    if (count < 0) {
        qmlInfo(this) << tr("remove: invalid count");
    } else if (index < 0 || index + count > model->m_compositor.count(d->group)) {
        qmlInfo(this) << tr("remove: index out of range");
    } else if (count > 0) {
        model->removeGroups(d->group, index, count, 1 << d->group);
    }
}

bool QQuickVisualDataGroupPrivate::parseGroupArgs(
        QDeclarativeV8Function *args, int *index, int *count, int *groups) const
{
    if (!model)
        return false;

    if (args->Length() < 2)
        return false;

    int i = 0;
    v8::Local<v8::Value> v = (*args)[i];
    if (!v->IsInt32())
        return false;
    *index = v->Int32Value();

    v = (*args)[++i];
    if (v->IsInt32()) {
        *count = v->Int32Value();

        if (++i == args->Length())
            return false;
        v = (*args)[i];
    }

    *groups = QQuickVisualDataModelPrivate::get(model)->m_cacheMetaType->parseGroups(args->engine(), v);

    return true;
}

/*!
    \qmlmethod QtQuick2::VisualDataGroup::addGroups(int index, int count, stringlist groups)

    Adds \a count items starting at \a index to \a groups.
*/

void QQuickVisualDataGroup::addGroups(QDeclarativeV8Function *args)
{
    Q_D(QQuickVisualDataGroup);
    int index = -1;
    int count = 1;
    int groups = 0;

    if (!d->parseGroupArgs(args, &index, &count, &groups))
        return;

    QQuickVisualDataModelPrivate *model = QQuickVisualDataModelPrivate::get(d->model);
    if (count < 0) {
        qmlInfo(this) << tr("addGroups: invalid count");
    } else if (index < 0 || index + count > model->m_compositor.count(d->group)) {
        qmlInfo(this) << tr("addGroups: index out of range");
    } else if (count > 0 && groups) {
        model->addGroups(d->group, index, count, groups);
    }
}

/*!
    \qmlmethod QtQuick2::VisualDataGroup::removeGroups(int index, int count, stringlist groups)

    Removes \a count items starting at \a index from \a groups.
*/

void QQuickVisualDataGroup::removeGroups(QDeclarativeV8Function *args)
{
    Q_D(QQuickVisualDataGroup);
    int index = -1;
    int count = 1;
    int groups = 0;

    if (!d->parseGroupArgs(args, &index, &count, &groups))
        return;

    QQuickVisualDataModelPrivate *model = QQuickVisualDataModelPrivate::get(d->model);
    if (count < 0) {
        qmlInfo(this) << tr("removeGroups: invalid count");
    } else if (index < 0 || index + count > model->m_compositor.count(d->group)) {
        qmlInfo(this) << tr("removeGroups: index out of range");
    } else if (count > 0 && groups) {
        model->removeGroups(d->group, index, count, groups);
    }
}

/*!
    \qmlmethod QtQuick2::VisualDataGroup::setGroups(int index, int count, stringlist groups)

    Sets the \a groups \a count items starting at \a index belong to.
*/

void QQuickVisualDataGroup::setGroups(QDeclarativeV8Function *args)
{
    Q_D(QQuickVisualDataGroup);
    int index = -1;
    int count = 1;
    int groups = 0;

    if (!d->parseGroupArgs(args, &index, &count, &groups))
        return;

    QQuickVisualDataModelPrivate *model = QQuickVisualDataModelPrivate::get(d->model);
    if (count < 0) {
        qmlInfo(this) << tr("setGroups: invalid count");
    } else if (index < 0 || index + count > model->m_compositor.count(d->group)) {
        qmlInfo(this) << tr("setGroups: index out of range");
    } else if (count > 0) {
        model->setGroups(d->group, index, count, groups);
    }
}

/*!
    \qmlmethod QtQuick2::VisualDataGroup::setGroups(int index, int count, stringlist groups)

    Sets the \a groups \a count items starting at \a index belong to.
*/

/*!
    \qmlmethod QtQuick2::VisualDataGroup::move(int from, int to, int count)

    Moves \a count at \a from in a group \a to a new position.
*/

void QQuickVisualDataGroup::move(QDeclarativeV8Function *args)
{
    Q_D(QQuickVisualDataGroup);

    if (args->Length() < 2)
        return;

    Compositor::Group fromGroup = d->group;
    Compositor::Group toGroup = d->group;
    int from = -1;
    int to = -1;
    int count = 1;

    int i = 0;
    v8::Local<v8::Value> v = (*args)[i];
    if (QQuickVisualDataGroup *group = qobject_cast<QQuickVisualDataGroup *>(args->engine()->toQObject(v))) {
        QQuickVisualDataGroupPrivate *g_d = QQuickVisualDataGroupPrivate::get(group);
        if (g_d->model != d->model)
            return;
        fromGroup = g_d->group;
        v = (*args)[++i];
    }

    if (!v->IsInt32())
        return;
    from = v->Int32Value();

    if (++i == args->Length())
        return;
    v = (*args)[i];

    if (QQuickVisualDataGroup *group = qobject_cast<QQuickVisualDataGroup *>(args->engine()->toQObject(v))) {
        QQuickVisualDataGroupPrivate *g_d = QQuickVisualDataGroupPrivate::get(group);
        if (g_d->model != d->model)
            return;
        toGroup = g_d->group;

        if (++i == args->Length())
            return;
        v = (*args)[i];
    }

    if (!v->IsInt32())
        return;
    to = v->Int32Value();

    if (++i < args->Length()) {
        v = (*args)[i];
        if (v->IsInt32())
            count = v->Int32Value();
    }

    QQuickVisualDataModelPrivate *model = QQuickVisualDataModelPrivate::get(d->model);

    if (count < 0) {
        qmlInfo(this) << tr("move: invalid count");
    } else if (from < 0 || from + count > model->m_compositor.count(fromGroup)) {
        qmlInfo(this) << tr("move: from index out of range");
    } else if (!model->m_compositor.verifyMoveTo(fromGroup, from, toGroup, to, count)) {
        qmlInfo(this) << tr("move: to index out of range");
    } else if (count > 0) {
        QVector<Compositor::Remove> removes;
        QVector<Compositor::Insert> inserts;

        model->m_compositor.move(fromGroup, from, toGroup, to, count, &removes, &inserts);
        model->itemsMoved(removes, inserts);
        model->emitChanges();
    }
}

/*!
    \qmlsignal QtQuick2::VisualDataGroup::onChanged(array removed, array inserted)

    This handler is called when items have been removed from or inserted into the group.

    Each object in the \a removed and \a inserted arrays has two values; the \e index of the first
    item inserted or removed and a \e count of the number of consecutive items inserted or removed.

    Each index is adjusted for previous changes with all removed items preceding any inserted
    items.
*/

//============================================================================

QQuickVisualPartsModel::QQuickVisualPartsModel(QQuickVisualDataModel *model, const QString &part, QObject *parent)
    : QQuickVisualModel(*new QObjectPrivate, parent)
    , m_model(model)
    , m_part(part)
    , m_compositorGroup(Compositor::Cache)
    , m_inheritGroup(true)
{
    QQuickVisualDataModelPrivate *d = QQuickVisualDataModelPrivate::get(m_model);
    if (d->m_cacheMetaType) {
        QQuickVisualDataGroupPrivate::get(d->m_groups[1])->emitters.insert(this);
        m_compositorGroup = Compositor::Default;
    } else {
        d->m_pendingParts.insert(this);
    }
}

QQuickVisualPartsModel::~QQuickVisualPartsModel()
{
}

QString QQuickVisualPartsModel::filterGroup() const
{
    if (m_inheritGroup)
        return m_model->filterGroup();
    return m_filterGroup;
}

void QQuickVisualPartsModel::setFilterGroup(const QString &group)
{
    if (QQuickVisualDataModelPrivate::get(m_model)->m_transaction) {
        qmlInfo(this) << tr("The group of a VisualDataModel cannot be changed within onChanged");
        return;
    }

    if (m_filterGroup != group || m_inheritGroup) {
        m_filterGroup = group;
        m_inheritGroup = false;
        updateFilterGroup();

        emit filterGroupChanged();
    }
}

void QQuickVisualPartsModel::resetFilterGroup()
{
    if (!m_inheritGroup) {
        m_inheritGroup = true;
        updateFilterGroup();
        emit filterGroupChanged();
    }
}

void QQuickVisualPartsModel::updateFilterGroup()
{
    QQuickVisualDataModelPrivate *model = QQuickVisualDataModelPrivate::get(m_model);
    if (!model->m_cacheMetaType)
        return;

    if (m_inheritGroup)
        return;

    QDeclarativeListCompositor::Group previousGroup = model->m_compositorGroup;
    m_compositorGroup = Compositor::Default;
    QQuickVisualDataGroupPrivate::get(model->m_groups[Compositor::Default])->emitters.insert(this);
    for (int i = 1; i < model->m_groupCount; ++i) {
        if (m_filterGroup == model->m_cacheMetaType->groupNames.at(i - 1)) {
            m_compositorGroup = Compositor::Group(i);
            break;
        }
    }

    QQuickVisualDataGroupPrivate::get(model->m_groups[m_compositorGroup])->emitters.insert(this);
    if (m_compositorGroup != previousGroup) {
        QVector<QDeclarativeChangeSet::Remove> removes;
        QVector<QDeclarativeChangeSet::Insert> inserts;
        model->m_compositor.transition(previousGroup, m_compositorGroup, &removes, &inserts);

        QDeclarativeChangeSet changeSet;
        changeSet.apply(removes, inserts);
        if (!changeSet.isEmpty())
            emit modelUpdated(changeSet, false);

        if (changeSet.difference() != 0)
            emit countChanged();
    }
}

void QQuickVisualPartsModel::updateFilterGroup(
        Compositor::Group group, const QDeclarativeChangeSet &changeSet)
{
    if (!m_inheritGroup)
        return;

    m_compositorGroup = group;
    QQuickVisualDataGroupPrivate::get(QQuickVisualDataModelPrivate::get(m_model)->m_groups[m_compositorGroup])->emitters.insert(this);

    if (!changeSet.isEmpty())
        emit modelUpdated(changeSet, false);

    if (changeSet.difference() != 0)
        emit countChanged();

    emit filterGroupChanged();
}

int QQuickVisualPartsModel::count() const
{
    QQuickVisualDataModelPrivate *model = QQuickVisualDataModelPrivate::get(m_model);
    return model->m_delegate
            ? model->m_compositor.count(m_compositorGroup)
            : 0;
}

bool QQuickVisualPartsModel::isValid() const
{
    return m_model->isValid();
}

QQuickItem *QQuickVisualPartsModel::item(int index, bool complete)
{
    QQuickVisualDataModelPrivate *model = QQuickVisualDataModelPrivate::get(m_model);

    if (!model->m_delegate || index < 0 || index >= model->m_compositor.count(m_compositorGroup)) {
        qWarning() << "VisualDataModel::item: index out range" << index << model->m_compositor.count(m_compositorGroup);
        return 0;
    }

    QObject *object = model->object(m_compositorGroup, index, complete, true);

    if (QDeclarativePackage *package = qobject_cast<QDeclarativePackage *>(object)) {
        QObject *part = package->part(m_part);
        if (!part)
            return 0;
        if (QQuickItem *item = qobject_cast<QQuickItem *>(part)) {
            m_packaged.insertMulti(item, package);
            return item;
        }
    }

    if (m_model->completePending())
        m_model->completeItem();
    model->release(object);
    if (!model->m_delegateValidated) {
        if (object)
            qmlInfo(model->m_delegate) << tr("Delegate component must be Package type.");
        model->m_delegateValidated = true;
    }

    return 0;
}

QQuickVisualModel::ReleaseFlags QQuickVisualPartsModel::release(QQuickItem *item)
{
    QQuickVisualModel::ReleaseFlags flags = 0;

    QHash<QObject *, QDeclarativePackage *>::iterator it = m_packaged.find(item);
    if (it != m_packaged.end()) {
        QDeclarativePackage *package = *it;
        QQuickVisualDataModelPrivate *model = QQuickVisualDataModelPrivate::get(m_model);
        flags = model->release(package);
        m_packaged.erase(it);
        if (!m_packaged.contains(item))
            flags &= ~Referenced;
        if (flags & Destroyed)
            QQuickVisualDataModelPrivate::get(m_model)->emitDestroyingPackage(package);
    }
    return flags;
}

bool QQuickVisualPartsModel::completePending() const
{
    return m_model->completePending();
}

void QQuickVisualPartsModel::completeItem()
{
    m_model->completeItem();
}

QString QQuickVisualPartsModel::stringValue(int index, const QString &role)
{
    return QQuickVisualDataModelPrivate::get(m_model)->stringValue(m_compositorGroup, index, role);
}

void QQuickVisualPartsModel::setWatchedRoles(QList<QByteArray> roles)
{
    QQuickVisualDataModelPrivate *model = QQuickVisualDataModelPrivate::get(m_model);
    model->m_adaptorModel->replaceWatchedRoles(m_watchedRoles, roles);
    m_watchedRoles = roles;
}

int QQuickVisualPartsModel::indexOf(QQuickItem *item, QObject *) const
{
    QHash<QObject *, QDeclarativePackage *>::const_iterator it = m_packaged.find(item);
    if (it != m_packaged.end()) {
        const QQuickVisualDataModelPrivate *model = QQuickVisualDataModelPrivate::get(m_model);
        const int cacheIndex = model->cacheIndexOf(*it);
        return cacheIndex != -1
                ? model->m_cache.at(cacheIndex)->index[m_compositorGroup]
                : -1;
    }
    return -1;
}

void QQuickVisualPartsModel::createdPackage(int index, QDeclarativePackage *package)
{
    if (QQuickItem *item = qobject_cast<QQuickItem *>(package->part(m_part)))
        emit createdItem(index, item);
}

void QQuickVisualPartsModel::destroyingPackage(QDeclarativePackage *package)
{
    if (QQuickItem *item = qobject_cast<QQuickItem *>(package->part(m_part))) {
        Q_ASSERT(!m_packaged.contains(item));
        emit destroyingItem(item);
    }
}

void QQuickVisualPartsModel::emitModelUpdated(const QDeclarativeChangeSet &changeSet, bool reset)
{
    emit modelUpdated(changeSet, reset);
    if (changeSet.difference() != 0)
        emit countChanged();
}


QT_END_NAMESPACE

#include <qquickvisualdatamodel.moc>
