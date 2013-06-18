/*
  LICENSE AND COPYRIGHT INFORMATION - Please read carefully.

  Copyright (c) 2010-2013, davyjones <dj@pgxplorer.com>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#ifndef TABLEVIEW_H
#define TABLEVIEW_H

#include <QtWidgets/QtWidgets>
#include <QtConcurrent/QtConcurrent>
#include <QSqlTableModel>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlIndex>
#include <QSqlError>
#include <QtGui>
#include "database.h"
#include "tablemodel.h"
#include "comboheader.h"

#define FETCHSIZ 10000

class TableView;
class ComboHeader;
class NewRowTableView;

class TableView : public QMainWindow
{
    Q_OBJECT

private:
    QString time_elapsed_string;
    QString rows_string;
    QString rows_string_2;
    QString colums_string;
    QString seconds_string;

    double time_elapsed;

    Database *database;
    QString table_name;
    QStringList primary_key;
    //bool primary_key_with_oid;
    QStringList column_list;
    QStringList column_types;
    QStringList column_lengths;
    QStringList current_column_functions;
    QList<QStringList> column_functions;
    QMenuBar *menubar;
    QDockWidget *dock_widget;
    QString status_message;
    QMenu context_menu;
    QMenu deselect_menu;
    QMenu disarrange_menu;
    QMenu ungroup_menu;
    QTime t;
    ToolBar *toolbar;
    TableModel *table_model;
    QTableView *table_view;
    QStandardItemModel *new_row_model;
    NewRowTableView *new_row_view;
    QString sql;
    QStringList where_clause;
    QStringList order_clause;
    QStringList group_clause;
    QStringList window_clause;
    QString limit;
    QStringList offset_list;
    bool quick_fetch;
    bool can_fetch_more;
    qint32 rows_from;
    qint32 rows_to;
    qint32 column_count;
    bool thread_busy;
    ulong thisTableViewId;
    const QIcon key_icon = QIcon(":/icons/key.png");
    const QIcon filter_icon = QIcon(":/icons/filter.png");
    const QIcon exclude_icon = QIcon(":/icons/exclude.png");
    const QIcon group_icon = QIcon(":/icons/group.png");
    const QIcon window_icon = QIcon(":/icons/window.png");
    const QIcon ascend_icon = QIcon(":/icons/ascending.png");
    const QIcon descend_icon = QIcon(":/icons/descending.png");
    QAction *default_action;
    QAction *refresh_action;
    QAction *copy_action;
    QAction *copy_with_headers_action;
    QAction *remove_columns_action;
    QAction *filter_action;
    QAction *exclude_action;
    QAction *group_action;
    QAction *window_action;
    QAction *ascend_action;
    QAction *descend_action;
    QAction *remove_all_filters_action;
    QAction *remove_all_ordering_action;
    QAction *remove_all_grouping_action;
    QAction *truncate_action;
    QAction *delete_rows_action;
    QAction *previous_set_action;
    QAction *next_set_action;
    QWidgetAction *custom_filter_action;
    QWidgetAction *bulk_update_action;
    QAction *copy_query_action;
    QLineEdit *filter_text;
    QLineEdit *bulk_update;
    QBrush red_brush;
    QBrush green_brush;
    QToolButton *previous_set_button;
    QToolButton *next_set_button;
    bool error_status;
    QMessageBox *error_message_box;
    ComboHeader *combo_header;

public:
    static ulong tableViewObjectId;
    TableView(Database *, QString const, QString const, QStringList const, QStringList const, QStringList const, QStringList const, bool, Qt::WidgetAttribute f);
    ~TableView()
    {
    };

    bool eventFilter(QObject *, QEvent*);
    void keyReleaseEvent(QKeyEvent*);
    //void contextMenuEvent(QContextMenuEvent*);
    void closeEvent(QCloseEvent*);

    void createBrushes();
    void createActions();
    void fetchConditionDataInitial();
    void deleteData();
    QString tableName()
    {
        return table_name;
    }

    ToolBar* getToolbar()
    {
        return toolbar;
    }

public slots:
    void languageChanged(QEvent*);
    void customContextMenuViewport();
    void customContextMenuHeader();
    void displayErrorMessage(QString);
    void bringOnTop();
    void selectColumn(int col)
    {
        table_view->selectColumn(col);
    }

private slots:
    void buildTableQuery();
    void fetchDefaultData();
    void fetchRefreshData(QString);
    void fetchDataSlot();
    void fetchNextData();
    void fetchPreviousData();
    void bulkUpdateData(QModelIndexList, QVariant);
    void copyToClipboard();
    void copyToClipboardWithHeaders();
    void defaultView();
    void refreshView();
    void addRowRefreshView();
    void filter();
    void filter(QString);
    void exclude();
    void group();
    void window();
    void ascend();
    void descend();
    void removeAllFilters();
    void removeAllGrouping();
    void removeAllOrdering();
    void removeColumns();
    void customFilterReturnPressed();
    void copyQuery();
    bool insertRow();
    void truncateTable();
    void deleteRows();
    void deleteRow(int);
    void updatePrimaryKeyInfo();
    void bulkUpdate();

    void busySlot();
    void notBusySlot();
    void updRowCntSlot(QString);
    void fullscreen();
    void restore();
    void toggleActions();
    void enableActions();
    void disableActions();

signals:
    void busySignal();
    void notBusySignal();
    void updRowCntSignal(QString);
    void queryFailed(QString);
    void functionsUpdated(QList<QStringList>);
    void tableViewClosing(TableView*);
};

class NewRowTableView : public QTableView
{
    Q_OBJECT

private:
    int column_count;

public:
    NewRowTableView(QWidget *parent = 0);
    void setColumnCount(quint32);
    bool eventFilter(QObject *, QEvent *);

public slots:
    void resizeCells(int, int, int);

signals:
    void insertRow();
};

#endif // TABLEVIEW_H
