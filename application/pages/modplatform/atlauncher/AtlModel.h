#pragma once

#include <QAbstractListModel>

#include "net/NetJob.h"
#include <QIcon>
#include <modplatform/atlauncher/ATLPackIndex.h>

namespace Atl {

typedef QMap<QString, QIcon> LogoMap;
typedef std::function<void(QString)> LogoCallback;

class ListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    ListModel(QObject *parent);
    virtual ~ListModel();

    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;

    void request();

    void getLogo(const QString &logo, const QString &logoUrl, LogoCallback callback);
    void searchWithTerm(const QString & term);

private slots:
    void performSearch();
    void searchRequestFinished();
    void searchRequestFailed(QString reason);

    void requestFinished();
    void requestFailed(QString reason);

    void requestPacks();

    void logoFailed(QString logo);
    void logoLoaded(QString logo, QIcon out);

private:
    void requestLogo(QString file, QString url);

private:
    QList<ATLauncher::IndexedPack> modpacks;
    QList<ATLauncher::IndexedPack> cachedpacks;

    QString currentSearchTerm;

    enum SearchState {
        None,
        CanPossiblyFetchMore,
        ResetRequested,
        Finished
    } searchState = None;

    QList<int> remainingPacks;

    QStringList m_failedLogos;
    QStringList m_loadingLogos;
    LogoMap m_logoMap;
    QMap<QString, LogoCallback> waitingCallbacks;

    NetJobPtr jobPtr;
    QByteArray response;
};

}
