#include "src/include/ui/processselectiondialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QCheckBox>
#include <QLabel>

ProcessSelectionDialog::ProcessSelectionDialog(const QList<ProcessInfo> &processes, const QString &title, QWidget *parent)
    : QDialog(parent), m_processes(processes)
{
    setupUi(title);

    for (const auto &process : m_processes) {
        QListWidgetItem *item = new QListWidgetItem(m_processListWidget);
        QWidget *widget = new QWidget();
        QHBoxLayout *layout = new QHBoxLayout(widget);
        layout->setContentsMargins(5, 2, 5, 2);

        QCheckBox *checkBox = new QCheckBox(this);
        QLabel *nameLabel = new QLabel(QString("%1 (PID: %2)").arg(process.name).arg(process.pid), this);
        QLabel *usageLabel = new QLabel(process.usageString, this);
        usageLabel->setFixedWidth(100); // Adjust width as needed
        usageLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

        layout->addWidget(checkBox);
        layout->addWidget(nameLabel, 1); // Stretch name label
        layout->addWidget(usageLabel);
        widget->setLayout(layout);

        item->setSizeHint(widget->sizeHint());
        m_processListWidget->addItem(item);
        m_processListWidget->setItemWidget(item, widget);
        m_itemToPidMap[item] = process.pid;
    }

    connect(m_okButton, &QPushButton::clicked, this, &ProcessSelectionDialog::onOkClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &ProcessSelectionDialog::onCancelClicked);
}

ProcessSelectionDialog::~ProcessSelectionDialog()
{
}

void ProcessSelectionDialog::setupUi(const QString &title)
{
    setWindowTitle(title);
    setMinimumSize(450, 300);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QLabel *instructionLabel = new QLabel(tr("请选择要关闭的进程:"), this);
    mainLayout->addWidget(instructionLabel);

    m_processListWidget = new QListWidget(this);
    m_processListWidget->setSelectionMode(QAbstractItemView::NoSelection);
    mainLayout->addWidget(m_processListWidget);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    m_okButton = new QPushButton(tr("确定"), this);
    m_cancelButton = new QPushButton(tr("取消"), this);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_okButton);
    buttonLayout->addWidget(m_cancelButton);

    mainLayout->addLayout(buttonLayout);
    setLayout(mainLayout);
}

QList<quint64> ProcessSelectionDialog::getSelectedPids() const
{
    return m_selectedPids;
}

void ProcessSelectionDialog::onOkClicked()
{
    m_selectedPids.clear();
    for (int i = 0; i < m_processListWidget->count(); ++i) {
        QListWidgetItem *item = m_processListWidget->item(i);
        QWidget *widget = m_processListWidget->itemWidget(item);
        if (widget) {
            QCheckBox *checkBox = widget->findChild<QCheckBox *>();
            if (checkBox && checkBox->isChecked()) {
                if (m_itemToPidMap.contains(item)) {
                    m_selectedPids.append(m_itemToPidMap[item]);
                }
            }
        }
    }
    accept();
}

void ProcessSelectionDialog::onCancelClicked()
{
    reject();
}