#include "tasklist.h"

#include <QVariant>
#include <QJsonArray>
#include <QtNetwork/QNetworkReply>
#include <QDebug>
#include <QJsonDocument>
#include <QSize>
#include <QBrush>
#include <QIcon>

TreeItem::TreeItem(TreeItem *parentItem): m_parentItem(parentItem)
{

}

TreeItem::~TreeItem()
{
    for (auto i: m_childItems)
    {
        delete i;
    }
}

void TreeItem::appendChild(TreeItem *child)
{
    m_childItems.append(child);
}

TreeItem *TreeItem::child(int row)
{
    return m_childItems[row];
}

int TreeItem::childCount() const
{
    return m_childItems.count();
}

int TreeItem::columnCount() const
{
    return 1;
}

QVariant TreeItem::data(int column) const
{
    Q_UNUSED(column);
    return {};
}

bool TreeItem::setData(const QVariant &/*value*/, int /*role*/)
{
    return false;
}

int TreeItem::row() const
{
    return m_parentItem ? m_parentItem->m_childItems.indexOf(const_cast<TreeItem*>(this)) : 0;
}

TreeItem *TreeItem::parentItem()
{
    return m_parentItem;
}

TaskList::TaskList(const QJsonObject &taskListObject, QOAuth2AuthorizationCodeFlow *flow, TreeItem *parent):
    etag(taskListObject["etag"].toString()),
    id(taskListObject["id"].toString()),
    kind(taskListObject["kind"].toString()),
    selfLink(taskListObject["selfLink"].toString()),
    title(taskListObject["title"].toString()),
    updated(taskListObject["updated"].toVariant().toDateTime()),
    mParent(parent)
{
    auto z = "https://www.googleapis.com/tasks/v1/lists/" + this->id + "/tasks";
    auto rest2 = flow->get(z);
    rest2->connect(rest2, &QNetworkReply::finished, [=]() {
        rest2->deleteLater();
        if (rest2->error() != QNetworkReply::NoError) {
            qCritical() << "Google error:" << rest2->errorString() << rest2->error();
            return;
        }

        const auto json = rest2->readAll();

        const auto document = QJsonDocument::fromJson(json);
        Q_ASSERT(document.isObject());
        const auto rootObject = document.object();
        auto jsons = rootObject.find("items")->toArray();
        for (auto i: jsons)
        {
            appendChild(new Task(i.toObject(), this));
        }
    });
}

void TaskList::appendChild(TreeItem *child)
{
    m_childItems.append(child);
}

TreeItem *TaskList::child(int row)
{
    if (row < 0 || row >= m_childItems.size())
            return nullptr;
    return m_childItems.at(row);
}

int TaskList::childCount() const
{
    return m_childItems.count();
}

int TaskList::columnCount() const
{
    return 1;
}

QVariant TaskList::data(int column) const
{
    return !column ? title : QVariant{};
}

int TaskList::row() const
{
    return 0;
}

TreeItem *TaskList::parentItem()
{
    return mParent;
}

Task::Task(const QJsonObject &taskObject, TreeItem *parent):
    kind(taskObject["kind"].toString()),
    id(taskObject["id"].toString()),
    etag(taskObject["etag"].toString()),
    mTitle(taskObject["title"].toString()),
    updated(taskObject["updated"].toVariant().toDateTime()),
    selfLink(taskObject["selfLink"].toString()),
    position(taskObject["position"].toVariant().toULongLong()),
    status(taskObject["status"].toString()),
    mParent(parent)
{

}

QString Task::getStatus() const
{
    return status;
}


TreeModel::TreeModel(const QJsonArray &data, std::shared_ptr<QOAuth2AuthorizationCodeFlow> flow, QObject *parent)
    : QAbstractItemModel(parent)
{
    rootItem = new TreeItem();
    setupModelData(data, flow.get(), rootItem);
}

TreeModel::~TreeModel()
{
    delete rootItem;
}

int TreeModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return static_cast<TreeItem*>(parent.internalPointer())->columnCount();
    return rootItem->columnCount();
}

QVariant TreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.internalPointer() == rootItem)
        return QVariant();

    switch (role) {
    case Qt::DisplayRole:
        return static_cast<TreeItem*>(index.internalPointer())->data(index.column());
    case Qt::SizeHintRole:
    {
        QSize size{};
        size.setHeight(dynamic_cast<Task*>(reinterpret_cast<TreeItem*>(index.internalPointer())) ? 25 : 50);
        return size;
    }
    case Qt::CheckStateRole:
    {
        if (auto x = dynamic_cast<Task*>(reinterpret_cast<TreeItem*>(index.internalPointer())); x != nullptr)
        {
            return x->getStatus() == "completed" ? Qt::Checked : Qt::Unchecked;
        }
        else
        {
            return {};
        }
    }
    case Qt::ForegroundRole:
    {
        return QBrush{QColor{Qt::black}};
    }
    case Qt::DecorationRole:
    {
        if (dynamic_cast<TaskList*>(reinterpret_cast<TreeItem*>(index.internalPointer())) != nullptr)
        {
            return QIcon{":/resources/google-tasks-icon.png"};
        }
        else
        {
            return {};
        }
    }
    default:
        return QVariant{};
    }
}

bool TreeModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    return index.isValid() && reinterpret_cast<TreeItem*>(index.internalPointer())->setData(value, role);
}

Qt::ItemFlags TreeModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;
    auto flags = QAbstractItemModel::flags(index);
    if (dynamic_cast<Task*>(reinterpret_cast<TreeItem*>(index.internalPointer())))
    {
        flags |= (Qt::ItemIsUserCheckable | Qt::ItemIsEditable);
    }

    return flags;
}

QVariant TreeModel::headerData(int section, Qt::Orientation orientation,
                               int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return rootItem->data(section);

    return QVariant();
}

QModelIndex TreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    TreeItem *parentItem;

    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<TreeItem*>(parent.internalPointer());

    TreeItem *childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);
    return QModelIndex();
}

QModelIndex TreeModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    TreeItem *childItem = static_cast<TreeItem*>(index.internalPointer());
    TreeItem *parentItem = childItem->parentItem();

    if (parentItem == rootItem)
        return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}

int TreeModel::rowCount(const QModelIndex &parent) const
{
    TreeItem *parentItem;
    if (parent.column() > 0)
        return 0;

    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<TreeItem*>(parent.internalPointer());

    return parentItem->childCount();
}

void TreeModel::setupModelData(const QJsonArray &lines, QOAuth2AuthorizationCodeFlow * flow, TreeItem *parent)
{
    for (const auto & i: lines)
    {
        parent->appendChild(new TaskList(i.toObject(), flow, parent));
    }
}
