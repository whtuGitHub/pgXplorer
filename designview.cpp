/*
  LICENSE AND COPYRIGHT INFORMATION - Please read carefully.

  Copyright (c) 2011-2012, davyjones <davyjones@github>

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

#include "designview.h"
#include "checkboxdelegate.h"

ulong DesignView::designViewObjectId = 0;

class CompleterDelegate : public QStyledItemDelegate
{
public:
    CompleterDelegate(QObject* parent = 0) : QStyledItemDelegate(parent) { }

    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
        QLineEdit* lineEdit = new QLineEdit(parent);
        QStringList stringList;
        stringList << "bigint" << "bigserial" << "bit" << "bit varying"
                   << "boolean" << "box" << "bytea" << "character varying"
                   << "character" << "cidr" << "circle" << "date"
                   << "double precision" << "inet" << "integer" << "interval"
                   << "line" << "lseg" << "macaddr" << "money"
                   << "numeric" << "path" << "point" << "polygon"
                   << "real" << "smallint" << "serial" << "text" << "time"
                   << "time without time zone" << "timestamp"
                   << "timestamp without time zone" << "tsquery"
                   << "tsvector" << "txid_snapshot" << "uuid" << "xml";
        QCompleter* completer = new QCompleter(stringList, lineEdit);
        completer->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
        completer->setCaseSensitivity(Qt::CaseInsensitive);

        lineEdit->setCompleter(completer);
        return lineEdit;
    }
    void setEditorData(QWidget *editor, const QModelIndex &index) const
    {
        QStyledItemDelegate::setEditorData(editor, index);
    }
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
    {
        QStyledItemDelegate::setModelData(editor, model, index);
    }
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        QStyledItemDelegate::updateEditorGeometry(editor, option, index);
    }
};
/*
class TableProperties : public QDialog
{
    Q_OBJECT

private:
    QCheckBox *with_oid;
    QDialogButtonBox *button_box;

public:
    TableProperties();
};

TableProperties::TableProperties()
{
    QFont font("Helvetica [Cronyx]");

    with_oid = new QCheckBox(this);
    with_oid->setFont(font);

    button_box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    button_box->setCenterButtons(true);
    button_box->setFont(font);
    connect(button_box, SIGNAL(accepted()), this, SLOT(okslot()));
    connect(button_box, SIGNAL(rejected()), this, SLOT(close()));

    QFormLayout *form_layout = new QFormLayout;
    form_layout->setVerticalSpacing(15);
    form_layout->addRow(tr("With oids"), with_oid);
    form_layout->labelForField(with_oid)->setFont(font);
    form_layout->addRow(button_box);
    setLayout(form_layout);
}
*/
DesignView::DesignView(Database *database, Table *table, QString const table_name, QString const name, QStringList column_list, QStringList primary_key, QStringList column_types, QStringList column_lengths, QStringList column_nulls, bool read_only, Qt::WidgetAttribute f)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowIcon(QIcon(":/icons/design.png"));
    menuBar()->setVisible(false);
    createBrushes();
    createActions();

    error_status = false;
    error_message_box = new QMessageBox(this);
    error_message_box->setWindowModality(Qt::WindowModal);

    this->database = database;
    this->table = table;
    this->table_name = table_name;
    this->primary_key = primary_key;
    this->column_list = column_list;
    this->column_types = column_types;
    this->column_lengths = column_lengths;
    this->column_nulls = column_nulls;

    setWindowTitle(name);
    setObjectName(name);

    toolbar = new QToolBar;
    toolbar->setIconSize(QSize(36,36));
    toolbar->setObjectName("designview");
    toolbar->setMovable(false);
    toolbar->addAction(save_action);
    toolbar->addSeparator();
    toolbar->addAction(properties_action);

    addToolBar(toolbar);
    statusBar()->show();

    //Identify this object with thisTableViewId for constructing database connection
    //specific to this object and this object alone.
    thisDesignViewId = designViewObjectId++;

    //Thread busy indicator to avoid overlapping of threads.
    //Initialise to false because obviously we don't have TableView
    //GUI artifacts to create overlapping threads yet.
    thread_busy = false;

    setContextMenuPolicy(Qt::NoContextMenu);

    design_model = new QStandardItemModel(6, column_list.size()+1);

    QModelIndex idx;
    for (int column = 0; column < column_list.size(); ++column) {
        QString column_name = column_list.at(column);
        QStandardItem *item = new QStandardItem(column_name.remove("\""));
             design_model->setItem(0, column, item);
             item = new QStandardItem(column_types.at(column));
             design_model->setItem(1, column, item);
             idx = design_model->index(2, column, QModelIndex());
             design_model->setData(idx, false);
             foreach(QString key_element, primary_key) {
                 int actual_index = column_list.indexOf(key_element);
                 if(actual_index > -1) {
                     QModelIndex idx = design_model->index(2, actual_index, QModelIndex());
                     design_model->setData(idx, true);
                 }
             }
             if(column_nulls.at(column).compare("true") == 0) {
                 idx = design_model->index(3, column, QModelIndex());
                 design_model->setData(idx, true);
             }
             else {
                 idx = design_model->index(3, column, QModelIndex());
                 design_model->setData(idx, false);
             }
    }
    QStandardItem *item = new QStandardItem(QString(""));
    design_model->setItem(0, column_list.size(), item);
    idx = design_model->index(2, column_list.size(), QModelIndex());
    design_model->setData(idx, false);
    idx = design_model->index(0, column_list.size(), QModelIndex());
    design_model->setData(idx, red_brush, Qt::BackgroundRole);
    idx = design_model->index(1, column_list.size(), QModelIndex());
    design_model->setData(idx, red_brush, Qt::BackgroundRole);

    design_model->setHeaderData(0, Qt::Vertical, tr("Name"));
    design_model->setHeaderData(1, Qt::Vertical, tr("Type"));
    design_model->setHeaderData(2, Qt::Vertical, tr("Primary key"));
    design_model->setHeaderData(3, Qt::Vertical, tr("Not null"));
    design_model->setHeaderData(4, Qt::Vertical, tr("Default value"));
    design_model->setHeaderData(5, Qt::Vertical, tr("Comment"));

    design_view = new QTableView(this);
    design_view->horizontalHeader()->setContextMenuPolicy(Qt::ActionsContextMenu);
    CheckBoxDelegate *check_delegate = new CheckBoxDelegate(design_view);
    design_view->setItemDelegateForRow(2, check_delegate);
    design_view->setItemDelegateForRow(3, check_delegate);
    CompleterDelegate *completer_delegate = new CompleterDelegate(design_view);
    design_view->setItemDelegateForRow(1, completer_delegate);
    design_view->viewport()->installEventFilter(this);
    design_view->installEventFilter(this);
    design_view->setAlternatingRowColors(true);
    design_view->verticalHeader()->setDefaultAlignment(Qt::AlignRight | Qt::AlignVCenter);
    design_view->setModel(design_model);
    connect(design_model, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(updateDesigner(QModelIndex,QModelIndex)));

    setCentralWidget(design_view);
}

void DesignView::updateDesigner(QModelIndex from, QModelIndex to)
{
    if(design_model->columnCount() > 1)
        save_action->setEnabled(true);
    QModelIndex idx = design_model->index(0, design_model->columnCount()-1, QModelIndex());
    QModelIndex idx2 = design_model->index(1, design_model->columnCount()-1, QModelIndex());
    if(!design_model->data(idx).toString().isEmpty() && !design_model->data(idx2).toString().isEmpty()) {
        QList<QStandardItem *> new_column;
        new_column << new QStandardItem("");
        design_model->appendColumn(new_column);
        new_column_list.append(design_model->data(idx).toString());
        new_column_types.append(design_model->data(idx2).toString());
        idx = design_model->index(0, design_model->columnCount()-1, QModelIndex());
        design_model->setData(idx, red_brush, Qt::BackgroundRole);
        idx = design_model->index(1, design_model->columnCount()-1, QModelIndex());
        design_model->setData(idx, red_brush, Qt::BackgroundRole);
        idx = design_model->index(0, design_model->columnCount()-2, QModelIndex());
        design_model->setData(idx, QBrush(), Qt::BackgroundRole);
        idx = design_model->index(1, design_model->columnCount()-2, QModelIndex());
        design_model->setData(idx, QBrush(), Qt::BackgroundRole);
    }

    if(from != to || from.row() != 2)
        return;

    if(from.data().toBool()) {
        idx = design_model->index(0, from.column(), QModelIndex());
        new_primary_key.append(idx.data().toString());
    }
    else {
        idx = design_model->index(0, from.column(), QModelIndex());
        new_primary_key.removeOne(idx.data().toString());
    }
}

void DesignView::createBrushes()
{
    QLinearGradient red_lineargradient(0, 0, 1.0, 0.25);
    red_lineargradient.setCoordinateMode(QGradient::StretchToDeviceMode);
    red_lineargradient.setColorAt(0, QColor::fromRgb(0xDE,0x00,0x00,160));
    red_lineargradient.setColorAt(1, QColor::fromRgb(0xEF,255,255,160));
    red_brush = QBrush(red_lineargradient);

    QLinearGradient green_lineargradient(0, 0, 1.0, 0.25);
    green_lineargradient.setCoordinateMode(QGradient::StretchToDeviceMode);
    green_lineargradient.setColorAt(0, QColor::fromRgb(0x00,0xDE,0x00,160));
    green_lineargradient.setColorAt(1, QColor::fromRgb(255,0xEF,255,160));
    green_brush = QBrush(green_lineargradient);
}

void DesignView::createActions()
{
    save_action = new QAction(QIcon(":/icons/save.png"), MainWin::tr("&Save"), this);
    save_action->setShortcuts(QKeySequence::Save);
    save_action->setStatusTip(MainWin::tr("Save the table definition to database"));
    save_action->setEnabled(false);
    connect(save_action, SIGNAL(triggered()), this, SLOT(saveTable()));

    properties_action = new QAction(QIcon(":/icons/properties.png"), MainWin::tr("&Properties"), this);
    properties_action->setStatusTip(MainWin::tr("Specify table properties"));
    connect(properties_action, SIGNAL(triggered()), this, SLOT(showProperties()));
}

void DesignView::saveTable()
{
    design_view->setDisabled(true);
    {
        QSqlDatabase database_connection = QSqlDatabase::addDatabase("QPSQL", QString("designview ").append(table_name));
        database_connection.setHostName(database->getHost());
        database_connection.setPort(database->getPort().toInt());
        database_connection.setDatabaseName(database->getName());
        database_connection.setUserName(database->getUser());
        database_connection.setPassword(database->getPassword());
        if (!database_connection.open()) {
            QMessageBox::critical(this, MainWin::tr("Database error"),
                MainWin::tr("Unable to establish a database connection.\n"
                            "No PostgreSQL support.\n"), QMessageBox::Cancel);
            return;
        }
        sql = QLatin1String("DROP TABLE IF EXISTS ");
        sql.append(table_name);
        sql.append(QLatin1String("; CREATE TABLE "));
        sql.append(table_name);
        sql.append(QLatin1String(" ("));
        int column_count = design_model->columnCount();
        for(int col = 0; col < column_count-1; col++) {
            QModelIndex idx0 = design_model->index(0, col, QModelIndex());
            QModelIndex idx1 = design_model->index(1, col, QModelIndex());
            QModelIndex idx2 = design_model->index(2, col, QModelIndex());
            QModelIndex idx3 = design_model->index(3, col, QModelIndex());
            sql.append(design_model->data(idx0).toString());
            sql.append(QLatin1String(" "));
            sql.append(design_model->data(idx1).toString());
            if(design_model->data(idx3).toBool())
                sql.append(QLatin1String(" NOT NULL "));
            sql.append(QLatin1String(", "));
        }
        if(new_primary_key.isEmpty()) {
            sql.remove(sql.length()-2, 2);
        }
        else {
            sql.append("CONSTRAINT \"");
            sql.append(QString(table_name).remove("\""));
            sql.append("_key\" PRIMARY KEY");
            sql.append(new_primary_key.join(",").prepend("(").append(")"));
        }
        sql.append(QLatin1String(") "));
        qDebug() << sql;
        QSqlQueryModel query;
        query.setQuery(sql, database_connection);
        if(query.lastError().isValid()) {
            QStringList messages = query.lastError().databaseText().split("\n");
            messages.removeLast();
            QMessageBox::critical(this, MainWin::tr("Database error"),
            messages.join("\n"), QMessageBox::Close);
        }
        else
            table->getParent()->resetTablesVertically2();
    }
    QSqlDatabase::removeDatabase(QString("designview ").append(table_name));
    design_view->setDisabled(false);
}

void DesignView::bringOnTop()
{
    activateWindow();
    raise();
}

void DesignView::showProperties()
{
    //TableProperties *table_props = new TableProperties;
    //table_props->exec();
}

void DesignView::languageChanged(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        save_action->setText(MainWin::tr("&Save"));
        save_action->setStatusTip(MainWin::tr("Save the table definition to database"));
        properties_action->setText(MainWin::tr("&Properties"));
        properties_action->setStatusTip(MainWin::tr("Specify table properties"));
        design_model->setHeaderData(0, Qt::Vertical, tr("Name"));
        design_model->setHeaderData(1, Qt::Vertical, tr("Type"));
        design_model->setHeaderData(2, Qt::Vertical, tr("Primary key"));
        design_model->setHeaderData(3, Qt::Vertical, tr("Not null"));
        design_model->setHeaderData(4, Qt::Vertical, tr("Default value"));
        design_model->setHeaderData(5, Qt::Vertical, tr("Comment"));
    }
}

void DesignView::closeEvent(QCloseEvent *event)
{
    event->accept();
    //Clean-up only when there is no active thread.
    //However, this will cause a memory leak when the
    //TableView is closed when the thread is busy.
    //Proper solution is to create a Thread class
    //and cancel that before we clean-up. We cannot do
    //this now because we are using QFuture (per Qt docs).
    emit designViewClosing(this);

    QSettings settings("pgXplorer", "pgXplorer");
    if(isMaximized()) {
        settings.setValue("designview_maximized", true);
        showNormal();
    }
    else
        settings.setValue("designview_maximized", false);
    settings.setValue("designview_pos", pos());
    settings.setValue("designview_size", size());

    delete toolbar;
    delete design_view;
    delete design_model;
    QMainWindow::closeEvent(event);
}
