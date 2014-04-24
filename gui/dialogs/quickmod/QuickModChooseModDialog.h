#pragma once

#include <QDialog>

#include "logic/BaseInstance.h"

namespace Ui
{
class QuickModChooseModDialog;
}

class QItemSelection;
class QListView;
class ModFilterProxyModel;
class CheckboxProxyModel;
class QuickMod;

class QuickModChooseModDialog : public QDialog
{
	Q_OBJECT

public:
	explicit QuickModChooseModDialog(InstancePtr instance, QWidget *parent = 0);
	~QuickModChooseModDialog();

private
slots:
	void on_installButton_clicked();
	void on_closeButton_clicked();
	void on_categoriesLabel_linkActivated(const QString &link);
	void on_tagsLabel_linkActivated(const QString &link);
	void on_fulltextEdit_textChanged();
	void on_tagsEdit_textChanged();
	void on_categoryBox_currentTextChanged();
	void on_addButton_clicked();
	void on_updateButton_clicked();
	void modSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

	void modLogoUpdated();

	void setupCategoryBox();

private:
	Ui::QuickModChooseModDialog *ui;

	QuickMod *m_currentMod;

	InstancePtr m_instance;

	QListView *m_view;
	ModFilterProxyModel *m_filterModel;
	CheckboxProxyModel *m_checkModel;
};
