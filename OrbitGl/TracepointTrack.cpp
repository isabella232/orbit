// Copyright (c) 2020 The Orbit Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "TracepointTrack.h"

#include "App.h"

TracepointTrack::TracepointTrack(TimeGraph* time_graph, int32_t thread_id)
    : EventTrack(time_graph) {
  thread_id_ = thread_id;
}

void TracepointTrack::Draw(GlCanvas* canvas, PickingMode picking_mode) {
  if (!HasTracepoints()) {
    return;
  }

  Batcher* batcher = canvas->GetBatcher();

  const float eventBarZ = picking_mode == PickingMode::kClick ? GlCanvas::kZValueEventBarPicking
                                                              : GlCanvas::kZValueEventBar;
  Color color = color_;
  Box box(pos_, Vec2(size_[0], -size_[1]), eventBarZ);
  batcher->AddBox(box, color, shared_from_this());

  if (canvas->GetPickingManager().IsThisElementPicked(this)) {
    color = Color(255, 255, 255, 255);
  }

  float x0 = pos_[0];
  float y0 = pos_[1];
  float x1 = x0 + size_[0];
  float y1 = y0 - size_[1];

  batcher->AddLine(pos_, Vec2(x1, y0), GlCanvas::kZValueEventBar, color, shared_from_this());
  batcher->AddLine(Vec2(x1, y1), Vec2(x0, y1), GlCanvas::kZValueEventBar, color,
                   shared_from_this());

  if (picked_) {
    Vec2& from = mouse_pos_[0];
    Vec2& to = mouse_pos_[1];

    x0 = from[0];
    y0 = pos_[1];
    x1 = to[0];
    y1 = y0 - size_[1];

    Color picked_color(0, 128, 255, 128);
    Box box(Vec2(x0, y0), Vec2(x1 - x0, -size_[1]), GlCanvas::kZValueUi);
    batcher->AddBox(box, picked_color, shared_from_this());
  }

  canvas_ = canvas;
}

void TracepointTrack::UpdatePrimitives(uint64_t min_tick, uint64_t max_tick,
                                       PickingMode picking_mode) {
  Batcher* batcher = &time_graph_->GetBatcher();
  const TimeGraphLayout& layout = time_graph_->GetLayout();
  float z = GlCanvas::kZValueEvent;
  float track_height = layout.GetEventTrackHeight();
  const bool picking = picking_mode != PickingMode::kNone;

  const Color kWhite(255, 255, 255, 255);

  const Color kWhiteTransparent(255, 255, 255, 190);

  const Color kGreenSelection(0, 255, 0, 255);

  if (!picking) {
    GOrbitApp->GetCaptureData().ForEachTracepointEventOfThreadInTimeRange(
        thread_id_, min_tick, max_tick,
        [&](const orbit_client_protos::TracepointEventInfo& tracepoint) {
          uint64_t time = tracepoint.time();
          float radius = track_height / 4;
          Vec2 pos(time_graph_->GetWorldFromTick(time), pos_[1]);
          batcher->AddVerticalLine(pos, -radius, z, kWhiteTransparent);
          batcher->AddVerticalLine(Vec2(pos[0], pos[1] - track_height), radius, z,
                                   kWhiteTransparent);
          batcher->AddCircle(Vec2(pos[0], pos[1] - track_height / 2), radius, z, kWhiteTransparent);
        });

  } else {
    constexpr float kPickingBoxWidth = 9.0f;
    constexpr float kPickingBoxOffset = kPickingBoxWidth / 2.0f;

    GOrbitApp->GetCaptureData().ForEachTracepointEventOfThreadInTimeRange(
        thread_id_, min_tick, max_tick,
        [&](const orbit_client_protos::TracepointEventInfo& tracepoint) {
          uint64_t time = tracepoint.time();
          Vec2 pos(time_graph_->GetWorldFromTick(time) - kPickingBoxOffset,
                   pos_[1] - track_height + 1);
          Vec2 size(kPickingBoxWidth, track_height);
          auto user_data = std::make_unique<PickingUserData>(
              nullptr, [&](PickingId id) -> std::string { return GetSampleTooltip(id); });
          user_data->custom_data_ = &tracepoint;
          batcher->AddShadedBox(pos, size, z, kGreenSelection, std::move(user_data));
        });
  }
}

void TracepointTrack::SetPos(float x, float y) {
  y = y - GetHeight();
  pos_ = Vec2(x, y);

  thread_name_.SetPos(pos_);
  thread_name_.SetSize(Vec2(size_[0] * 0.3f, size_[1]));
}

float TracepointTrack::GetHeight() const {
  TimeGraphLayout& layout = time_graph_->GetLayout();

  return HasTracepoints() ? layout.GetEventTrackHeight() + layout.GetSpaceBetweenTracksAndThread()
                          : 0;
}

bool TracepointTrack::HasTracepoints() const {
  return !GOrbitApp->GetCaptureData().GetTracepointsOfThread(thread_id_).empty();
}

void TracepointTrack::OnPick(int x, int y) {
  Vec2& mouse_pos = mouse_pos_[0];
  canvas_->ScreenToWorld(x, y, mouse_pos[0], mouse_pos[1]);
  mouse_pos_[1] = mouse_pos_[0];
  picked_ = true;
}

void TracepointTrack::OnRelease() { picked_ = false; }

std::string TracepointTrack::GetSampleTooltip(PickingId id) const {
  auto user_data = time_graph_->GetBatcher().GetUserData(id);
  CHECK(user_data && user_data->custom_data_);

  const auto* tracepoint_event_info =
      static_cast<const orbit_client_protos::TracepointEventInfo*>(user_data->custom_data_);

  uint64_t tracepoint_info_key = tracepoint_event_info->tracepoint_info_key();

  TracepointInfo tracepoint_info =
      GOrbitApp->GetCaptureData().GetTracepointInfo(tracepoint_info_key);

  return absl::StrFormat(
      "<b>Tracepoint event</b><br/>"
      "<br/>"
      "<b>Core:</b> %d<br/>"
      "<b>Name:</b> %s [%s]<br/>",
      tracepoint_event_info->cpu(), tracepoint_info.name(), tracepoint_info.category());
}