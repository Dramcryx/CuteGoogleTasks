#ifndef TASKLIST_H
#define TASKLIST_H

#include <memory>

#include <QObject>
#include <QJsonObject>
#include <QDateTime>
#include <QUrl>
#include <QVector>
#include <QAbstractItemModel>
#include <QtNetworkAuth/QOAuth2AuthorizationCodeFlow>

class TreeItem
{
public:
    TreeItem(TreeItem *parentItem = nullptr);
    virtual ~TreeItem();

    virtual void appendChild(TreeItem *child);

    virtual TreeItem *child(int row);
    virtual int childCount() const;
    inline int columnCount() const;
    virtual QVariant data(int column) const;
    virtual bool setData(const QVariant & value, int role);
    virtual int row() const;
    virtual TreeItem *parentItem();

    QVector<TreeItem*> m_childItems;
    QVector<QVariant> m_itemData;
    TreeItem *m_parentItem;
};



class TaskList;

class Task: public TreeItem
{
public:
    Task(const QJsonObject & taskObject, TreeItem *parent);

    virtual void appendChild(TreeItem *child)
    {
        Q_UNUSED(child)
    }

    virtual TreeItem *child(int row)
    {
        Q_UNUSED(row)
        return nullptr;
    }

    virtual int childCount() const
    {
        return 0;
    }

    inline int columnCount() const
    {
        return 1;
    }
    virtual QVariant data(int column) const
    {
        return column ? QVariant{} : mTitle;
    }

    virtual bool setData(const QVariant & value, int role)
    {
        switch (role)
        {
        case Qt::CheckStateRole:
        {
            status = value.toBool() ? "completed" : "needsAction";
            break;
        }
        case Qt::EditRole:
        {
            mTitle = value.toString();
            break;
        }
        default:
            return false;
        }
        return true;
    }

    virtual int row() const
    {
        if (m_parentItem)
            return m_parentItem->m_childItems.indexOf(const_cast<Task*>(this));

        return 0;
    }
    virtual TreeItem *parentItem()
    {
        return mParent;
    }

    inline QString title() const
    {
        return mTitle;
    }

    QString getStatus() const;

private:
    QString kind;
    QString id;
    QString etag;
    QString mTitle;
    QDateTime updated;
    QUrl selfLink;
    unsigned long position;
    QString status;

    TreeItem * mParent;
};

class TaskList: public TreeItem
{
public:
    TaskList(const QJsonObject & taskListObject, QOAuth2AuthorizationCodeFlow * flow, TreeItem *parent);

    virtual void appendChild(TreeItem *child);

    virtual TreeItem *child(int row);
    virtual int childCount() const;
    virtual int columnCount() const;
    virtual QVariant data(int column) const;
    virtual bool setData(const QVariant & value, int role)
    {
        switch (role)
        {
        case Qt::EditRole:
        {
            title = value.toString();
            break;
        }
        default:
            return false;
        }
        return true;
    }
    virtual int row() const;
    virtual TreeItem *parentItem();
private:
    QString   etag;
    QString   id;
    QString   kind;
    QString   selfLink;
    QString   title;
    QDateTime updated;

    TreeItem * mParent;
    QVector<TreeItem*> m_childItems;
};

class TreeModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit TreeModel(const QJsonArray &data, std::shared_ptr<QOAuth2AuthorizationCodeFlow> flow, QObject *parent = nullptr);
    ~TreeModel();

    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

private:
    void setupModelData(const QJsonArray &lines, QOAuth2AuthorizationCodeFlow *flow, TreeItem *parent);

    TreeItem *rootItem;
};

#endif // TASKLIST_H
