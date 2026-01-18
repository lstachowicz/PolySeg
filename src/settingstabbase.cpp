#include "settingstabbase.h"

BaseSettingsTab::BaseSettingsTab(QWidget* parent)
    : QWidget(parent), scroll_area_(nullptr), content_widget_(nullptr), main_layout_(nullptr)
{
}

void BaseSettingsTab::SetupScrollArea()
{
  // Create the main layout for this tab widget
  QVBoxLayout* tab_layout = new QVBoxLayout(this);
  tab_layout->setContentsMargins(0, 0, 0, 0);

  // Create scroll area
  scroll_area_ = new QScrollArea();
  scroll_area_->setWidgetResizable(true);
  scroll_area_->setFrameShape(QFrame::NoFrame);

  // Create scrollable content widget
  content_widget_ = new QWidget();
  main_layout_ = new QVBoxLayout(content_widget_);

  // Setup scroll area
  scroll_area_->setWidget(content_widget_);
  tab_layout->addWidget(scroll_area_);
}
