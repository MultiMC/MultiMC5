/* Copyright 2013 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "QuickMod.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QVariant>

#include "logic/net/CacheDownload.h"
#include "logic/net/NetJob.h"
#include "MultiMC.h"
#include "QuickModVersion.h"
#include "QuickModsList.h"
#include "modutils.h"
#include "logic/MMCJson.h"

QuickModUid::QuickModUid() : m_uid(QString())
{
}
QuickModUid::QuickModUid(const QString &uid) : m_uid(uid)
{
}
QString QuickModUid::toString() const
{
	return m_uid;
}
QuickModPtr QuickModUid::mod() const
{
	return mods().first();
}
QList<QuickModPtr> QuickModUid::mods() const
{
	return MMC->quickmodslist()->mods(*this);
}
QDebug operator<<(QDebug &dbg, const QuickModUid &uid)
{
	dbg.nospace() << uid.toString();
	return dbg.maybeSpace();
}
uint qHash(const QuickModUid &uid)
{
	return qHash(uid.toString());
}

QuickMod::QuickMod(QObject *parent) : QObject(parent), m_imagesLoaded(false)
{
}

QuickMod::~QuickMod()
{
	m_versions.clear(); // as they are shared pointers this will also delete them
}

QIcon QuickMod::icon()
{
	fetchImages();
	return m_icon;
}
QPixmap QuickMod::logo()
{
	fetchImages();
	return m_logo;
}

QuickModVersionPtr QuickMod::version(const QString &name) const
{
	for (QuickModVersionPtr ptr : m_versions)
	{
		if (ptr->name() == name)
		{
			return ptr;
		}
	}
	return QuickModVersionPtr();
}
QuickModVersionPtr QuickMod::latestVersion(const QString &mcVersion) const
{
	for (auto version : m_versions)
	{
		if (version->compatibleVersions.contains(mcVersion))
		{
			return version;
		}
	}
	return QuickModVersionPtr();
}

void QuickMod::parse(QuickModPtr _this, const QByteArray &data)
{
	const QJsonDocument doc = MMCJson::parseDocument(data, "QuickMod file");
	const QJsonObject mod = MMCJson::ensureObject(doc, "root");
	m_uid = QuickModUid(MMCJson::ensureString(mod.value("uid"), "'uid'"));
	m_repo = MMCJson::ensureString(mod.value("repo"), "'repo'");
	m_name = MMCJson::ensureString(mod.value("name"), "'name'");
	m_description = mod.value("description").toString();
	m_nemName = mod.value("nemName").toString();
	m_modId = mod.value("modId").toString();
	m_websiteUrl = Util::expandQMURL(mod.value("websiteUrl").toString());
	m_verifyUrl = Util::expandQMURL(mod.value("verifyUrl").toString());
	m_iconUrl = Util::expandQMURL(mod.value("iconUrl").toString());
	m_logoUrl = Util::expandQMURL(mod.value("logoUrl").toString());
	m_references.clear();
	const QJsonObject references = mod.value("references").toObject();
	for (auto key : references.keys())
	{
		m_references.insert(QuickModUid(key), Util::expandQMURL(MMCJson::ensureString(
												  references[key], "'reference'")));
	}
	m_categories.clear();
	for (auto val : mod.value("categories").toArray())
	{
		m_categories.append(MMCJson::ensureString(val, "'category'"));
	}
	m_tags.clear();
	for (auto val : mod.value("tags").toArray())
	{
		m_tags.append(MMCJson::ensureString(val, "'tag'"));
	}
	m_updateUrl =
		Util::expandQMURL(MMCJson::ensureString(mod.value("updateUrl"), "'updateUrl'"));

	m_versions.clear();
	for (auto val : MMCJson::ensureArray(mod.value("versions"), "'versions'"))
	{
		QuickModVersionPtr ptr(new QuickModVersion(_this));
		ptr->parse(MMCJson::ensureObject(val, "version"));
		m_versions.append(ptr);
	}
	qSort(m_versions.begin(), m_versions.end(),
		  [](const QuickModVersionPtr v1, const QuickModVersionPtr v2)
	{ return Util::Version(v1->name()) > Util::Version(v2->name()); });

	if (!m_uid.isValid())
	{
		throw QuickModParseError("The 'uid' field in the QuickMod file for " + m_name +
								 " is empty");
	}
	if (m_repo.isEmpty())
	{
		throw QuickModParseError("The 'repo' field in the QuickMod file for " + m_name +
								 " is empty");
	}
}

bool QuickMod::compare(const QuickModPtr other) const
{
	return m_name == other->m_name || m_uid == other->m_uid;
}

void QuickMod::iconDownloadFinished(int index)
{
	auto download = qobject_cast<CacheDownload *>(sender());
	m_icon = QIcon(download->getTargetFilepath());
	if (!m_icon.isNull())
	{
		emit iconUpdated();
	}
}
void QuickMod::logoDownloadFinished(int index)
{
	auto download = qobject_cast<CacheDownload *>(sender());
	m_logo = QPixmap(download->getTargetFilepath());
	if (!m_logo.isNull())
	{
		emit logoUpdated();
	}
}

void QuickMod::fetchImages()
{
	if (m_imagesLoaded)
	{
		return;
	}
	auto job = new NetJob("QuickMod image download: " + m_name);
	bool download = false;
	m_imagesLoaded = true;
	if (m_iconUrl.isValid() && m_icon.isNull())
	{
		auto icon = CacheDownload::make(
			m_iconUrl, MMC->metacache()->resolveEntry("quickmod/icons", fileName(m_iconUrl)));
		connect(icon.get(), &CacheDownload::succeeded, this, &QuickMod::iconDownloadFinished);
		job->addNetAction(icon);
		download = true;
	}
	if (m_logoUrl.isValid() && m_logo.isNull())
	{
		auto logo = CacheDownload::make(
			m_logoUrl, MMC->metacache()->resolveEntry("quickmod/logos", fileName(m_logoUrl)));
		connect(logo.get(), &CacheDownload::succeeded, this, &QuickMod::logoDownloadFinished);
		job->addNetAction(logo);
		download = true;
	}
	if (download)
	{
		job->start();
	}
	else
	{
		delete job;
	}
}

QString QuickMod::fileName(const QUrl &url) const
{
	const QString path = url.path();
	return internalUid() + path.mid(path.lastIndexOf("."));
}

QStringList QuickMod::mcVersions()
{
	if (m_mcVersionListCache.isEmpty())
	{
		QStringList mcvs;
		for (auto quickModV : versions())
		{
			for (QString mcv : quickModV->compatibleVersions)
			{
				if (!mcvs.contains(mcv))
					mcvs.append(mcv);
			}
		}
		m_mcVersionListCache << mcvs;
	}
	return m_mcVersionListCache;
}
