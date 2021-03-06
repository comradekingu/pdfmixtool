/* Copyright (C) 2017-2018 Marco Scarpetta
 *
 * This file is part of PDF Mix Tool.
 *
 * PDF Mix Tool is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * PDF Mix Tool is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PDF Mix Tool. If not, see <http://www.gnu.org/licenses/>.
 */

#include "mainwindow.h"

#include <QApplication>
#include <QTimer>
#include <QHeaderView>

#define NAME_COLUMN 0
#define PAGES_FILTER_COLUMN 2
#define ROTATION_COLUMN 3
#define PDF_FILE_ROLE Qt::UserRole
#define ROTATATION_ROLE Qt::UserRole + 1

Q_DECLARE_METATYPE(InputPdfFile *)

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    m_pdf_editor(new PdfEditor()),
    m_settings(new QSettings(this)),
    m_add_file_button(new QPushButton(QIcon::fromTheme("list-add"), tr("Add PDF file"), this)),
    m_move_up_button(new QPushButton(QIcon::fromTheme("go-up"), tr("Move up"), this)),
    m_move_down_button(new QPushButton(QIcon::fromTheme("go-down"), tr("Move Down"), this)),
    m_remove_file_button(new QPushButton(QIcon::fromTheme("list-remove"), tr("Remove file"), this)),
    m_about_button(new QPushButton(QIcon::fromTheme("help-about"), tr("About"), this)),
    m_generate_pdf_button(new QPushButton(QIcon::fromTheme("document-save-as"), tr("Generate PDF"), this)),
    m_output_page_count(new QLabel(this)),
    m_progress_bar(new QProgressBar(this)),
    m_open_file_dialog(new QFileDialog(this)),
    m_opened_count(0),
    m_dest_file_dialog(new QFileDialog(this)),
    m_files_list_view(new QTableView(this)),
    m_combobox_delegate(new ComboBoxDelegate(this)),
    m_lineedit_delegate(new LineEditDelegate(this)),
    m_files_list_model(new QStandardItemModel(0, 4, this)),
    m_error_dialog(new QMessageBox(this)),
    m_warning_dialog(new QMessageBox(this)),
    m_about_dialog(new AboutDialog(this))
{
    this->setWindowIcon(QIcon(QString("%1/../share/icons/hicolor/48x48/apps/pdfmixtool.png").arg(qApp->applicationDirPath())));
    this->setWindowTitle(tr("PDF Mix Tool"));
    this->restoreGeometry(m_settings->value("main_window_geometry").toByteArray());

    m_error_dialog->setIcon(QMessageBox::Critical);
    m_error_dialog->setTextFormat(Qt::RichText);

    m_warning_dialog->setIcon(QMessageBox::Warning);
    m_warning_dialog->setTextFormat(Qt::RichText);
    m_warning_dialog->addButton(QMessageBox::Cancel);
    QPushButton *ignore_warning = m_warning_dialog->addButton(QMessageBox::Ignore);
    m_warning_dialog->setDefaultButton(QMessageBox::Ignore);

    m_progress_bar->hide();

    m_open_file_dialog->setNameFilter(tr("PDF files (*.pdf)"));
    m_open_file_dialog->setFilter(QDir::Files);
    m_open_file_dialog->setFileMode(QFileDialog::ExistingFiles);
    m_open_file_dialog->setDirectory(m_settings->value("open_directory", "").toString());
    m_open_file_dialog->setModal(true);

    m_dest_file_dialog->setNameFilter(tr("PDF files (*.pdf)"));
    m_dest_file_dialog->setFilter(QDir::Files);
    m_dest_file_dialog->setAcceptMode(QFileDialog::AcceptSave);
    m_dest_file_dialog->setDirectory(m_settings->value("save_directory", "").toString());
    m_dest_file_dialog->setModal(true);

    m_files_list_view->setWordWrap(false);
    m_files_list_view->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_files_list_view->setEditTriggers(QAbstractItemView::CurrentChanged |
                                       QAbstractItemView::DoubleClicked |
                                       QAbstractItemView::SelectedClicked);
    m_files_list_view->setModel(m_files_list_model);
    m_files_list_model->setHorizontalHeaderItem(0, new QStandardItem(tr("Filename")));
    m_files_list_model->setHorizontalHeaderItem(1, new QStandardItem(tr("Page count")));
    m_files_list_model->setHorizontalHeaderItem(2, new QStandardItem(tr("Pages filter")));
    m_files_list_model->setHorizontalHeaderItem(3, new QStandardItem(tr("Rotation")));
    m_files_list_view->setItemDelegateForColumn(2, m_lineedit_delegate);
    m_files_list_view->setItemDelegateForColumn(3, m_combobox_delegate);
    m_files_list_view->horizontalHeader()->setStretchLastSection(true);
    m_files_list_view->resizeColumnsToContents();
    m_files_list_view->setFocusPolicy(Qt::ClickFocus);

    m_add_file_button->setDefault(true);
    m_move_up_button->setDefault(true);
    m_move_down_button->setDefault(true);
    m_remove_file_button->setDefault(true);
    m_about_button->setDefault(true);
    m_generate_pdf_button->setAutoDefault(true);

    QWidget::setTabOrder(m_add_file_button, m_move_up_button);
    QWidget::setTabOrder(m_move_up_button, m_move_down_button);
    QWidget::setTabOrder(m_move_down_button, m_remove_file_button);
    QWidget::setTabOrder(m_remove_file_button, m_about_button);
    QWidget::setTabOrder(m_about_button, m_generate_pdf_button);

    QVBoxLayout *v_layout = new QVBoxLayout();
    QWidget *central_widget = new QWidget(this);
    central_widget->setLayout(v_layout);
    this->setCentralWidget(central_widget);

    QHBoxLayout *layout = new QHBoxLayout();
    layout->addWidget(m_add_file_button);
    layout->addWidget(m_move_up_button);
    layout->addWidget(m_move_down_button);
    layout->addWidget(m_remove_file_button);
    layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));
    layout->addWidget(m_about_button);
    v_layout->addLayout(layout);

    v_layout->addWidget(m_files_list_view);

    layout = new QHBoxLayout();
    layout->addWidget(m_output_page_count);
    layout->addWidget(m_progress_bar, 1);
    layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));
    layout->addWidget(m_generate_pdf_button);
    v_layout->addLayout(layout);

    connect(m_add_file_button, SIGNAL(pressed()), m_open_file_dialog, SLOT(exec()));
    connect(m_open_file_dialog, SIGNAL(filesSelected(QStringList)), this, SLOT(pdf_file_added(QStringList)));

    connect(m_move_up_button, SIGNAL(pressed()), this, SLOT(move_up()));
    connect(m_move_down_button, SIGNAL(pressed()), this, SLOT(move_down()));
    connect(m_remove_file_button, SIGNAL(pressed()), this, SLOT(remove_pdf_file()));

    connect(m_about_button, SIGNAL(pressed()), m_about_dialog, SLOT(show()));

    connect(m_lineedit_delegate, SIGNAL(data_edit()), this, SLOT(update_output_page_count()));

    connect(m_generate_pdf_button, SIGNAL(pressed()), this, SLOT(generate_pdf_button_pressed()));

    connect(ignore_warning, SIGNAL(released()), m_dest_file_dialog, SLOT(show()));

    connect(m_dest_file_dialog, SIGNAL(fileSelected(QString)), this, SLOT(generate_pdf(QString)));
}

void MainWindow::pdf_file_added(const QStringList &selected)
{
    for (int i=m_opened_count; i<selected.count(); i++)
    {
        //check if filename is already in the model
        InputPdfFile * pdf_file = NULL;

        for (int j=0; j<m_files_list_model->rowCount(); j++)
        {
            QStandardItem *item = m_files_list_model->item(j, NAME_COLUMN);
            if (item->data(PDF_FILE_ROLE).value<InputPdfFile *>()->filename() == selected.at(i).toStdString())
                pdf_file = item->data(PDF_FILE_ROLE).value<InputPdfFile *>();
        }

        if (pdf_file != NULL)
            pdf_file = m_pdf_editor->new_input_file(pdf_file);
        else
            pdf_file = m_pdf_editor->new_input_file(selected.at(i).toStdString());

        //create the row
        QList<QStandardItem *> row_data;
        QVariant input_pdf_file_address = QVariant::fromValue<InputPdfFile *>(pdf_file);

        QStandardItem *column = new QStandardItem(selected.at(i));
        column->setEditable(false);
        column->setData(input_pdf_file_address, PDF_FILE_ROLE);
        row_data << column;

        column = new QStandardItem(QString::number(pdf_file->page_count()));
        column->setEditable(false);
        column->setData(input_pdf_file_address, PDF_FILE_ROLE);
        row_data << column;

        column = new QStandardItem();
        column->setData(input_pdf_file_address, PDF_FILE_ROLE);
        row_data << column;

        column = new QStandardItem(tr("No rotation"));
        column->setData(input_pdf_file_address, PDF_FILE_ROLE);
        column->setData(0, ROTATATION_ROLE);
        row_data << column;

        m_files_list_model->appendRow(row_data);
    }

    m_opened_count = selected.count();
    m_open_file_dialog->setDirectory(m_open_file_dialog->directory());
    m_dest_file_dialog->setDirectory(m_open_file_dialog->directory());
    m_settings->setValue("open_directory", m_open_file_dialog->directory().absolutePath());

    this->update_output_page_count();

    m_files_list_view->resizeColumnsToContents();
}

void MainWindow::move_up()
{
    if (m_files_list_view->selectionModel()->hasSelection())
    {
        int from = -1;
        int to = -1;

        QModelIndexList selected = m_files_list_view->selectionModel()->selectedRows();
        for (QModelIndexList::const_iterator it=selected.begin(); it<selected.end(); ++it)
        {
            if (from == -1)
                from = (*it).row();
            if (to == -1 || (*it).row() - to == 1)
                to = (*it).row();
            else
                return; //non contiguous rows
        }

        if (from > 0)
        {
            for (int i=from; i<=to; i++)
            {
                QList<QStandardItem *> row = m_files_list_model->takeRow(i);
                m_files_list_model->insertRow(i-1, row);
            }

            QItemSelection sel(m_files_list_model->index(from-1, 0), m_files_list_model->index(to-1, 0));
            m_files_list_view->selectionModel()->select(sel, QItemSelectionModel::Select | QItemSelectionModel::Rows);
        }
    }
}

void MainWindow::move_down()
{
    if (m_files_list_view->selectionModel()->hasSelection())
    {
        int from = -1;
        int to = -1;

        QModelIndexList selected = m_files_list_view->selectionModel()->selectedRows();
        for (QModelIndexList::const_iterator it=selected.begin(); it<selected.end(); ++it)
        {
            if (from == -1)
                from = (*it).row();
            if (to == -1 || (*it).row() - to == 1)
                to = (*it).row();
            else
                return; //non contiguous rows
        }

        if (to < m_files_list_model->rowCount() - 1)
        {
            for (int i=to; i>=from; i--)
            {
                QList<QStandardItem *> row = m_files_list_model->takeRow(i);
                m_files_list_model->insertRow(i+1, row);
            }

            QItemSelection sel(m_files_list_model->index(from+1, 0), m_files_list_model->index(to+1, 0));
            m_files_list_view->selectionModel()->select(sel, QItemSelectionModel::Select | QItemSelectionModel::Rows);
        }
    }
}

void MainWindow::remove_pdf_file()
{
    QList<int> rows;
    for(const QModelIndex & index : m_files_list_view->selectionModel()->selectedRows()) {
       rows.append(index.row());
    }

    qSort(rows);

    for (int i=rows.count() - 1; i >= 0; i--)
    {
        delete m_files_list_model->item(rows[i], NAME_COLUMN)->data(PDF_FILE_ROLE).value<InputPdfFile *>();
        m_files_list_model->removeRow(rows[i]);
    }

    this->update_output_page_count();
}

void MainWindow::update_output_page_count()
{
    int output_page_count = 0;
    for (int i=0; i<m_files_list_model->rowCount(); i++)
    {
        InputPdfFile *pdf_file = m_files_list_model->item(i, NAME_COLUMN)->data(PDF_FILE_ROLE).value<InputPdfFile *>();
        output_page_count += pdf_file->output_page_count();
    }

    m_output_page_count->setText(tr("Output pages: %1").arg(output_page_count));
}

void MainWindow::generate_pdf_button_pressed()
{
    //check if there is at least one input file
    if (m_files_list_model->rowCount() == 0)
    {
        m_error_dialog->setWindowTitle(tr("PDF generation error"));
        m_error_dialog->setText(tr("You must add at least one pdf file!"));
        m_error_dialog->show();
        return;
    }

    //set pages rotation and filters
    for (int i=0; i<m_files_list_model->rowCount(); i++)
    {
        InputPdfFile *pdf_file = m_files_list_model->item(i, NAME_COLUMN)->data(PDF_FILE_ROLE).value<InputPdfFile *>();

        pdf_file->set_rotation(m_files_list_model->item(i, ROTATION_COLUMN)->data(ROTATATION_ROLE).toInt());

        Problems problems = pdf_file->set_pages_filter_from_string(
                    m_files_list_model->item(i, PAGES_FILTER_COLUMN)->data(Qt::EditRole).toString().toStdString());

        if (problems.count != 0)
        {
            if (problems.errors)
            {
                m_error_dialog->setWindowTitle(tr("PDF generation error"));
                QString error_message(tr("<p>The PDF generation failed due to the following errors:</p>"));
                error_message += QString("<ul>");

                Problem *error = problems.first;
                Problem *next;

                while (error != NULL)
                {
                    next = error->next;

                    if (error->type == ProblemType::error_invalid_char)
                        error_message +=
                                tr("<li>Invalid character \"<b>%1</b>\" in pages filter of file \"<b>%2</b>\"</li>")
                                .arg(
                                    QString::fromStdString(error->data),
                                    m_files_list_model->item(i, NAME_COLUMN)->text());
                    else if (error->type == ProblemType::error_invalid_interval)
                        error_message +=
                                tr("<li>Invalid interval \"<b>%1</b>\" in pages filter of file \"<b>%2</b>\"</li>")
                                .arg(
                                    QString::fromStdString(error->data),
                                    m_files_list_model->item(i, NAME_COLUMN)->text());
                    else if (error->type == ProblemType::error_page_out_of_range)
                        error_message +=
                                tr("<li>Boundaries of interval \"<b>%1</b>\" in pages filter of file \"<b>%2</b>\" are out of allowed interval</li>")
                                .arg(
                                    QString::fromStdString(error->data),
                                    m_files_list_model->item(i, NAME_COLUMN)->text());

                    delete error;

                    error = next;
                }

                error_message += QString("</ul>");
                m_error_dialog->setText(error_message);
                m_error_dialog->show();
            }
            else
            {
                m_warning_dialog->setWindowTitle(tr("PDF generation warning"));
                QString warning_message(tr("<p>The following problems were encountered while generating the PDF file:</p>"));
                warning_message += QString("<ul>");

                Problem *warning = problems.first;
                Problem *next;

                while (warning != NULL)
                {
                    next = warning->next;

                    if (warning->type == ProblemType::warning_overlapping_interval)
                        warning_message +=
                                tr("<li>Interval \"<b>%1</b>\" in pages filter of file \"<b>%2</b>\" is overlapping with another interval</li>")
                                .arg(
                                    QString::fromStdString(warning->data),
                                    m_files_list_model->item(i, NAME_COLUMN)->text());

                    delete warning;

                    warning = next;
                }

                warning_message += QString("</ul>");
                m_warning_dialog->setText(warning_message);
                m_warning_dialog->show();
            }

            return;
        }
    }

    //show destination file dialog if there wasn't any error before
    m_dest_file_dialog->show();
}

void MainWindow::generate_pdf(const QString &file_selected)
{
    m_settings->setValue("save_directory", m_dest_file_dialog->directory().absolutePath());

    m_progress_bar->setValue(0);
    m_progress_bar->show();

    OutputPdfFile *output_file = m_pdf_editor->new_output_file();

    //write each file to the output file
    for (int i=0; i<m_files_list_model->rowCount(); i++)
    {
        InputPdfFile *pdf_file = m_files_list_model->item(i, NAME_COLUMN)->data(PDF_FILE_ROLE).value<InputPdfFile *>();

        pdf_file->run(output_file);

        m_progress_bar->setValue((i+1)*100/(m_files_list_model->rowCount()+1));
    }

    //save output file on disk
    output_file->write(file_selected.toStdString());
    delete output_file;

    m_progress_bar->setValue(100);
    QTimer::singleShot(2000, m_progress_bar, SLOT(hide()));
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    m_settings->setValue("main_window_geometry", this->saveGeometry());
    QMainWindow::closeEvent(event);
}
