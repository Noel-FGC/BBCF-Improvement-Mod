#pragma once
#include "ReplayRewindWindow.h"
#include "Core/interfaces.h"
#include "Game/SnapshotApparatus/SnapshotApparatus.h"
#include "Game/gamestates.h"
#include "Overlay/imgui_utils.h"

unsigned int ReplayRewindWindow::count_entities(bool unk_status2) {
    if (!g_interfaces.player1.IsCharDataNullPtr() && !g_interfaces.player2.IsCharDataNullPtr()) {
        std::vector<int*> entities{};
        std::vector<CharData**> entities_char_data{};
        for (int i = 0; i < 252; i++) {
            entities.push_back((g_gameVals.pEntityList + i));
            //entities_char_data.push_back((CharData*)(g_gameVals.pEntityList + i));
        }
        for (auto entity_ptr : entities) {
            entities_char_data.push_back((CharData**)entity_ptr);
        }
        auto tst = entities_char_data[0];
        auto r = 0;
        for (auto entity : entities_char_data) {
            if ((*entity)->unknownStatus1 != NULL) {
                if (unk_status2) {
                    if ((*entity)->unknown_status2 != NULL && (*entity)->unknown_status2 == 2) {
                        r++;
                    }
                }
                else {
                    r++;
                }
            }
        }
        return r;
        //auto tst = &entities_char_data[0]->frame_count_minus_1;
    }
    return 0;
}
std::vector<int> ReplayRewindWindow::find_nearest_checkpoint(std::vector<unsigned int> frameCount) {
    //returns a vector with the fist being the nearest pos in the checkpoints for a backwards and the second for the fwd, -1 if not available
    auto fc = *g_gameVals.pFrameCount;
    int nearest_back = 9999999;
    int nearest_back_pos = -1;
    int nearest_fwd = 9999999;
    int nearest_fwd_pos = -1;
    int i = 0;
    if (frameCount.size() == 0) {
        static_DAT_of_PTR_on_load_4* DAT_on_load_4_addr = (static_DAT_of_PTR_on_load_4*)(GetBbcfBaseAdress() + 0x612718);
        SnapshotManager* snap_manager = 0;
        snap_manager = DAT_on_load_4_addr->ptr_snapshot_manager_mine;
        auto stru = snap_manager->_saved_states_related_struct;
        frameCount = std::vector<unsigned int>{};
        for (int i = 0; (i < 10) && (stru[i]._framecount != 0); i++) {
            frameCount.push_back(stru[i]._framecount);
        }
    }
    for (auto frameCheckpoint : frameCount) {
        if ((fc - frameCheckpoint < fc - nearest_back && fc - frameCheckpoint >60)) {
            nearest_back = frameCheckpoint;
            nearest_back_pos = i;
        }
        if ((frameCheckpoint - fc < nearest_fwd - fc && frameCheckpoint - fc >60)) {
            nearest_fwd = frameCheckpoint;
            nearest_fwd_pos = i;
        }


        i++;
    }
    if (frameCount.empty()) {
        return std::vector<int>{-1, -1};
    }
    if (frameCount.size() < nearest_back_pos + 1) {
        nearest_fwd_pos = -1;
    }
    return std::vector<int>{nearest_back_pos, nearest_fwd_pos};
}
void ReplayRewindWindow::Draw()
{
    
#ifdef _DEBUG
    ImGui::Text("Active entities: %d", ReplayRewindWindow::count_entities(false));
    ImGui::Text("Active entities with unk_status2 = 2: %d", count_entities(true));
#endif
    static int prev_match_state;
    static bool rec = false;
    //static int first_checkpoint = 0;
    static int FIRST_CHECKPOINT_FRAME = 0;
    static  char LAST_SAVED_ROUND;// = *(bbcf_base_adress + 0x11C034C);
    static int FRAME_STEP = 540;
    static CharData* p1_to_check;// = g_interfaces.player1.GetData;
    static CharData* p2_to_check;// = g_interfaces.player1;
    auto bbcf_base_adress = GetBbcfBaseAdress();
    char* ptr_replay_theater_current_frame = bbcf_base_adress + 0x11C0348;
    static bool playing = false;
    static int curr_frame = *g_gameVals.pFrameCount;
    static int prev_frame;
    //static int frames_recorded = 0;
    static int rewind_pos = 0;
    static int round_start_frame = 0;
    static std::vector<unsigned int> frame_checkpoints = {};
    std::vector<unsigned int> frame_checkpoints_clipped;
    frame_checkpoints_clipped = std::vector<unsigned int>{};

    static SnapshotApparatus* snap_apparatus_replay_rewind = nullptr;
    if (*g_gameVals.pGameMode != GameMode_ReplayTheater || *g_gameVals.pGameState != GameState_InMatch) {
        ImGui::Text("Only works during a running replay");

        return;
    }
    if (*(bbcf_base_adress + 0x8F7758) == 0) {
        if (!g_interfaces.player1.IsCharDataNullPtr() && !g_interfaces.player2.IsCharDataNullPtr()) {
            if (snap_apparatus_replay_rewind == nullptr) {

                snap_apparatus_replay_rewind = new SnapshotApparatus();
            }
            if (!snap_apparatus_replay_rewind->check_if_valid(g_interfaces.player1.GetData(),
                g_interfaces.player2.GetData())) {
                delete snap_apparatus_replay_rewind;
                snap_apparatus_replay_rewind = new SnapshotApparatus();
            }
            /*if (*g_gameVals.pGameMode == GameMode_ReplayTheater && *g_gameVals.pMatchState == MatchState_Fight) {
                toggle_unknown2_asm_code();
            }*/
            curr_frame = *g_gameVals.pFrameCount;

            if ((*g_gameVals.pGameMode != GameMode_ReplayTheater && *g_gameVals.pGameMode != GameMode_Training)
                || (*g_gameVals.pMatchState != MatchState_Fight && *g_gameVals.pMatchState != MatchState_RebelActionRoundSign && *g_gameVals.pMatchState != MatchState_FinishSign)
                || *g_gameVals.pGameState != GameState_InMatch) {
                if (rec) {
                    rec = false;
                    //framestates = {};
                    snap_apparatus_replay_rewind->clear_count();
                    rewind_pos = 0;
                    frame_checkpoints.clear();
                    //force clear the vectors
                    return;
                }

            }







            //*ptr_replay_theater_current_frame = *g_gameVals.pFrameCount;

            //memcpy(ptr_replay_theater_current_frame, g_gameVals.pFrameCount, sizeof(unsigned int));

            ///grabs the frame count on round start
            if (*g_gameVals.pGameMode == GameMode_ReplayTheater && prev_match_state && prev_match_state == MatchState_RebelActionRoundSign &&
                *g_gameVals.pMatchState == MatchState_Fight && !g_interfaces.player1.IsCharDataNullPtr() && !g_interfaces.player2.IsCharDataNullPtr()) {
                round_start_frame = *g_gameVals.pFrameCount;

            }

            //makes sure that as long as you're in the replay and it is inMatch the recording is activated
            if ((*g_gameVals.pGameMode == GameMode_ReplayTheater && *g_gameVals.pMatchState == MatchState_Fight)
                && (rec == false
                    || (p2_to_check != g_interfaces.player2.GetData() && p1_to_check != g_interfaces.player1.GetData()) //this deals with inter replays
                    || (LAST_SAVED_ROUND != *(bbcf_base_adress + 0x11C034C))) //this deals with intra replays
                )
            {

                rewind_pos = 0;
                frame_checkpoints.clear();
                snap_apparatus_replay_rewind->clear_framecounts();
                snap_apparatus_replay_rewind->clear_count();
                rec = true;
                snap_apparatus_replay_rewind->save_snapshot(0);
                FIRST_CHECKPOINT_FRAME = *g_gameVals.pFrameCount;
                LAST_SAVED_ROUND = *(bbcf_base_adress + 0x11C034C);
                frame_checkpoints.push_back(*g_gameVals.pFrameCount);
                prev_frame = *g_gameVals.pFrameCount;
                p2_to_check = g_interfaces.player2.GetData();
                p1_to_check = g_interfaces.player1.GetData();

            }

            ///automatic start rec on round start + first snapshot there
            if (*g_gameVals.pFrameCount == round_start_frame && *g_gameVals.pMatchState == MatchState_Fight && FIRST_CHECKPOINT_FRAME == 0) {
                //snap_apparatus_replay_rewind->clear_framecounts();
                rec = true;
                snap_apparatus_replay_rewind->save_snapshot(0);
                FIRST_CHECKPOINT_FRAME = *g_gameVals.pFrameCount;
                LAST_SAVED_ROUND = *(bbcf_base_adress + 0x11C034C);
                frame_checkpoints.push_back(*g_gameVals.pFrameCount);
                prev_frame = *g_gameVals.pFrameCount;

            }



            //automatic clear vector if change round or leave abruptly, currently removing the reset between rounds
            if (*g_gameVals.pGameState != GameState_InMatch
                || (*g_gameVals.pGameMode == GameMode_ReplayTheater
                    && prev_match_state == MatchState_Fight
                    && *g_gameVals.pMatchState == MatchState_FinishSign
                    && !g_interfaces.player1.IsCharDataNullPtr()
                    && !g_interfaces.player2.IsCharDataNullPtr())
                ) {
                rec = false;
                //frames_recorded = 0;
                rewind_pos = 0;
                frame_checkpoints.clear();
                snap_apparatus_replay_rewind->clear_framecounts();
                snap_apparatus_replay_rewind->clear_count();
                round_start_frame = 0;
                FIRST_CHECKPOINT_FRAME = 0;
            }

            if (rec
                && (
                    (*g_gameVals.pGameMode != GameMode_Training
                        && *g_gameVals.pGameMode != GameMode_ReplayTheater)
                    || *g_gameVals.pMatchState != MatchState_Fight)
                ) {
                rec = false;
            }
            prev_match_state = *g_gameVals.pMatchState;
            ImGui::Text("Rewind Interval"); ImGui::SameLine(); ImGui::ShowHelpMarker("Defines the interval to save the rewind \"checkpoint\". For more info see the help bar on \"Rewind\" button");
            ImGui::RadioButton("1s", &FRAME_STEP, 60); ImGui::SameLine();
            ImGui::RadioButton("3s", &FRAME_STEP, 180); ImGui::SameLine();
            ImGui::RadioButton("9s", &FRAME_STEP, 540);
            ImGui::Separator();

            if (ImGui::TreeNode("Saved Checkpoints Advanced Info")) {
                ImGui::Text("Rewind pos: +%d", rewind_pos);
                auto nearest_pos = ReplayRewindWindow::find_nearest_checkpoint(frame_checkpoints_clipped);
                ImGui::Text("Rewind checkpoint: %d    FF checkpoint(nearest): %d", nearest_pos[0], nearest_pos[1]);
                ImGui::Text("snap_apparatus snapshot_count: %d", snap_apparatus_replay_rewind->snapshot_count);
                if (snap_apparatus_replay_rewind != nullptr) {
                    static_DAT_of_PTR_on_load_4* DAT_on_load_4_addr = (static_DAT_of_PTR_on_load_4*)(bbcf_base_adress + 0x612718);
                    SnapshotManager* snap_manager = 0;
                    snap_manager = DAT_on_load_4_addr->ptr_snapshot_manager_mine;
                    int iter = 0;
                    for (auto& state : snap_manager->_saved_states_related_struct) {

                        ImGui::Text("%d: Framecount:%d", iter, state._framecount);
                        iter++;
                    }

                }
                ImGui::TreePop();
            }

            if (*g_gameVals.pGameMode == GameMode_ReplayTheater) {
                if (ImGui::Button("Rewind")) {

                    int pos = ReplayRewindWindow::find_nearest_checkpoint(frame_checkpoints_clipped)[0];
                    if (pos != -1) {
                        snap_apparatus_replay_rewind->load_snapshot_index(pos);

                        //starts the replay
                        char* replay_theather_speed = bbcf_base_adress + 0x11C0350;
                        *replay_theather_speed = 0;
                        rewind_pos = pos;
                    }
                    // }
                }
                ImGui::SameLine();
                ImGui::ShowHelpMarker("Replay rewind can currently hold up to 10 \"checkpoints\" at a time to rewind to. These checkpoints are saved as the replay progresses at defined intervals(1s,3s or 9s). With replay interval set to 9s for example you will save checkpoints at second 0,9,18...,90.\n\
\n\
You can also change them during the replay itself to refine the rewind, say for example you found something interesting at second 16 while having rewind interval as 9s:\n\
\n\
\t - Rewind until you reach the checkpoint at second 9\n\
\t - Change the rewind interval to 1s or 3s\n\
\t - Now as it reaches the desired position it will have recorded the previous checkpoints in smaller intervals, allowing you to wait less to reach the desired part / analyze the lead up to it.\n\
\n\
You can see the frames of all saved checkpoints and more advanced info on the \"Saved Checkpoints Advanced Info\" section above.");
            }


            //Here is where the recording is done on the appropriate frames
            if (rec && *g_gameVals.pGameMode == GameMode_ReplayTheater) {
                auto save = true;
                for (auto saved_frame : frame_checkpoints) {
                    if (curr_frame == saved_frame) {
                        save = false;
                    }
                }
                if (((curr_frame - FIRST_CHECKPOINT_FRAME == 0) || (curr_frame - FIRST_CHECKPOINT_FRAME) % FRAME_STEP == 0)
                    && save == true) {

                    snap_apparatus_replay_rewind->save_snapshot(0);
                    frame_checkpoints.push_back(*g_gameVals.pFrameCount);
                    // frames_recorded += 1;
                    rewind_pos += 1;
                    prev_frame = curr_frame;
                }
            }

            return;
        }
    }
    else {
        ImGui::Text("You cannot use this feature while searching for a ranked match");
    }
}
