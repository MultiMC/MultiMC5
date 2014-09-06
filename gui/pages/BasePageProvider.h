/* Copyright 2014 MultiMC Contributors
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

#pragma once

#include "BasePage.h"
#include <memory>
#include <functional>

class BasePageProvider
{
public:
	virtual QList<BasePage *> getPages() = 0;
	virtual QString dialogTitle() = 0;
};

class GenericPageProvider : public BasePageProvider
{
	typedef std::function<BasePage *()> PageCreator;
public:
	explicit GenericPageProvider(const QString &dialogTitle)
		: m_dialogTitle(dialogTitle)
	{
	}

	QList<BasePage *> getPages() override
	{
		QList<BasePage *> pages;
		for (PageCreator creator : m_creators)
		{
			pages.append(creator());
		}
		return pages;
	}
	QString dialogTitle() override { return m_dialogTitle; }

	void setDialogTitle(const QString &title)
	{
		m_dialogTitle = title;
	}
	void addPageCreator(PageCreator page)
	{
		m_creators.append(page);
	}

	template<typename PageClass>
	void addPage()
	{
		addPageCreator([](){return new PageClass();});
	}
	template<typename PageClass, typename... Params>
	void addPage(Params... params)
	{
		addPageCreator(std::bind([](Params... params){return new PageClass(params...);}, params...));
	}

private:
	QList<PageCreator> m_creators;
	QString m_dialogTitle;
};

typedef std::shared_ptr<BasePageProvider> BasePageProviderPtr;
