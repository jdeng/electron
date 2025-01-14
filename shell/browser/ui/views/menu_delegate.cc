// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/views/menu_delegate.h"

#include <memory>

#include "base/task/post_task.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "shell/browser/ui/views/menu_bar.h"
#include "shell/browser/ui/views/menu_model_adapter.h"
#include "ui/views/controls/button/menu_button.h"
#include "ui/views/controls/menu/menu_item_view.h"
#include "ui/views/controls/menu/menu_runner.h"
#include "ui/views/widget/widget.h"

namespace electron {

MenuDelegate::MenuDelegate(MenuBar* menu_bar)
    : menu_bar_(menu_bar), id_(-1), hold_first_switch_(false) {}

MenuDelegate::~MenuDelegate() = default;

void MenuDelegate::RunMenu(AtomMenuModel* model,
                           views::Button* button,
                           ui::MenuSourceType source_type) {
  gfx::Point screen_loc;
  views::View::ConvertPointToScreen(button, &screen_loc);
  // Subtract 1 from the height to make the popup flush with the button border.
  gfx::Rect bounds(screen_loc.x(), screen_loc.y(), button->width(),
                   button->height() - 1);

  if (source_type == ui::MENU_SOURCE_KEYBOARD) {
    hold_first_switch_ = true;
  }

  id_ = button->tag();
  adapter_ = std::make_unique<MenuModelAdapter>(model);

  views::MenuItemView* item = new views::MenuItemView(this);
  static_cast<MenuModelAdapter*>(adapter_.get())->BuildMenu(item);

  menu_runner_ = std::make_unique<views::MenuRunner>(
      item, views::MenuRunner::CONTEXT_MENU | views::MenuRunner::HAS_MNEMONICS);
  menu_runner_->RunMenuAt(
      button->GetWidget()->GetTopLevelWidget(),
      static_cast<views::MenuButton*>(button)->button_controller(), bounds,
      views::MenuAnchorPosition::kTopRight, source_type);
}

void MenuDelegate::ExecuteCommand(int id) {
  for (Observer& obs : observers_)
    obs.OnBeforeExecuteCommand();
  adapter_->ExecuteCommand(id);
}

void MenuDelegate::ExecuteCommand(int id, int mouse_event_flags) {
  for (Observer& obs : observers_)
    obs.OnBeforeExecuteCommand();
  adapter_->ExecuteCommand(id, mouse_event_flags);
}

bool MenuDelegate::IsTriggerableEvent(views::MenuItemView* source,
                                      const ui::Event& e) {
  return adapter_->IsTriggerableEvent(source, e);
}

bool MenuDelegate::GetAccelerator(int id, ui::Accelerator* accelerator) const {
  return adapter_->GetAccelerator(id, accelerator);
}

base::string16 MenuDelegate::GetLabel(int id) const {
  return adapter_->GetLabel(id);
}

void MenuDelegate::GetLabelStyle(int id, LabelStyle* style) const {
  return adapter_->GetLabelStyle(id, style);
}

bool MenuDelegate::IsCommandEnabled(int id) const {
  return adapter_->IsCommandEnabled(id);
}

bool MenuDelegate::IsCommandVisible(int id) const {
  return adapter_->IsCommandVisible(id);
}

bool MenuDelegate::IsItemChecked(int id) const {
  return adapter_->IsItemChecked(id);
}

void MenuDelegate::WillShowMenu(views::MenuItemView* menu) {
  adapter_->WillShowMenu(menu);
}

void MenuDelegate::WillHideMenu(views::MenuItemView* menu) {
  adapter_->WillHideMenu(menu);
}

void MenuDelegate::OnMenuClosed(views::MenuItemView* menu) {
  for (Observer& obs : observers_)
    obs.OnMenuClosed();

  // Only switch to new menu when current menu is closed.
  if (button_to_open_)
    button_to_open_->Activate(nullptr);
  delete this;
}

views::MenuItemView* MenuDelegate::GetSiblingMenu(
    views::MenuItemView* menu,
    const gfx::Point& screen_point,
    views::MenuAnchorPosition* anchor,
    bool* has_mnemonics,
    views::MenuButton**) {
  if (hold_first_switch_) {
    hold_first_switch_ = false;
    return nullptr;
  }

  // TODO(zcbenz): We should follow Chromium's logics on implementing the
  // sibling menu switches, this code is almost a hack.
  views::MenuButton* button;
  AtomMenuModel* model;
  if (menu_bar_->GetMenuButtonFromScreenPoint(screen_point, &model, &button) &&
      button->tag() != id_) {
    bool switch_in_progress = !!button_to_open_;
    // Always update target to open.
    button_to_open_ = button;
    // Switching menu asyncnously to avoid crash.
    if (!switch_in_progress) {
      base::PostTaskWithTraits(
          FROM_HERE, {content::BrowserThread::UI},
          base::BindOnce(&views::MenuRunner::Cancel,
                         base::Unretained(menu_runner_.get())));
    }
  }

  return nullptr;
}

}  // namespace electron
