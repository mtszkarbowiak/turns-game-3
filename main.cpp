#include <iostream>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <functional>

using std::string;
using std::cout;
using std::cin;
using std::endl;
using std::ifstream;
using std::ofstream;
using std::vector;
using std::function;



namespace maths2{

    float clamp(float t, float min, float max){
        if(t > max) return max;
        if(t < min) return min;
        return t;
    }

}



namespace data_model{
    enum element{
        water = 0,   earth = 1,   air = 2,
        fire = 3,    ice = 4,     metal = 5,
        element_none = 7,
    };

    struct difficulty_t{
        string name;
        float out_dmg_mul;
        float in_dmg_mul;
        int enemy_count;
        int player_count;
    };

    struct evolution_meta_t{
        int creature_id;
        int level;

        string name;
        float strength;
        float max_health;
        float agility;
        float bounty_exp;
        float required_exp;

        int skill_type;
        float skill_power;

        evolution_meta_t* next_evolution;
    };

    struct creature_meta_t{
        int id;
        string name;
        element element;
    };

    enum player_action{
        player_action_none = 0,
        player_action_attack = 1,
        player_action_skill_use = 2,
        player_action_creature_reselection = 4,
        player_action_evolution = 8,
    };


    class creature_i{
    public:
        virtual float get_health() = 0;
        virtual float get_exp() = 0;
        virtual bool is_alive() = 0;
        virtual const evolution_meta_t* get_evolution() = 0;
        virtual const creature_meta_t* get_creature() = 0;

        bool can_evolute();
    };

    class team_i{
    public:
        virtual size_t get_creature_count() = 0;
        virtual creature_i* get_creature(int index) = 0;
        virtual creature_i* get_selected_creature() = 0;
        virtual bool is_creature_selectable(int index) = 0;

        virtual bool is_defeated() = 0;
    };

    class game_status_i{
    public:
        virtual int get_turn_index() = 0;
        virtual bool is_player_turn() = 0;
        virtual team_i* get_player_team() = 0;
        virtual size_t get_enemy_teams_count() = 0;
        virtual team_i* get_enemy_team(int index) = 0;

        bool are_all_enemies_defeated();
        bool is_game_over();
        int get_current_enemy_index();

        virtual bool can_make_turn_select_any_creature(bool player_team) = 0;
        virtual bool can_make_turn_select_creature(bool player_team, int selection_index) = 0;
        virtual bool can_make_turn_evolute(bool player_team) = 0;
        virtual bool can_make_turn_use_attack(bool player_team) = 0;
        virtual bool can_make_turn_use_skill(bool player_team) = 0;

        virtual void make_turn_select_creature(bool player_team, int selection_index) = 0;
        virtual void make_turn_evolute(bool player_team) = 0;
        virtual void make_turn_use_attack(bool player_team) = 0;
        virtual void make_turn_use_skill(bool player_team) = 0;

        virtual void swap_turns() = 0;
    };



    bool game_status_i::is_game_over() {
        return get_player_team()->is_defeated() || are_all_enemies_defeated();
    }

    bool game_status_i::are_all_enemies_defeated() {
        for (int i = 0; i < get_enemy_teams_count(); ++i) {
            auto enemy_team = get_enemy_team(i);
            if(!enemy_team->is_defeated())
                return false;
        }
        return true;
    }

    int game_status_i::get_current_enemy_index() {
        for (int i = 0; i < get_enemy_teams_count(); ++i) {
            auto enemy_team = get_enemy_team(i);
            if(!enemy_team->is_defeated())
                return i;
        }
        return -1;
    }

    bool creature_i::can_evolute() {
        return
                this->get_exp() >= this->get_evolution()->required_exp &&
                this->get_evolution()->next_evolution != nullptr &&
                this->is_alive();
    }
}



namespace data_importing{
    using namespace data_model;

    const vector<const difficulty_t*>* difficulties;
    const vector<const creature_meta_t*>* creatures;
    const vector<const evolution_meta_t*>* evolutions;

    const char* difficulties_file_name = "Difficulties.txt";
    const char* evolutions_file_name = "Evolutions.txt";
    const char* creatures_file_name = "Creatures.txt";

    const char* element_names[] {
            "Water", "Earth", "Air",
            "Fire",  "Ice",   "Metal",
            "None"
    };


    element get_element_by_name(const string &name) {
        int index = 0;
        for (const char* element_name : element_names) {
            auto result = name.compare(element_name);
            if(result == 0) return (element) index;
        }
        return element_none;
    }


    void load_difficulties() {
        auto* difficulties_temp = new vector<const difficulty_t*>;
        std::ifstream i(difficulties_file_name);
        while (!i.eof()){
            auto difficulty = new difficulty_t;
            i >> difficulty->name;
            i >> difficulty->out_dmg_mul;
            i >> difficulty->in_dmg_mul;
            i >> difficulty->enemy_count;
            i >> difficulty->player_count;
            difficulties_temp->push_back(difficulty);
        }
        i.close();
        difficulties = difficulties_temp;
    }

    void load_creatures() {
        auto* creatures_temp = new vector<const creature_meta_t*>;
        std::ifstream i(creatures_file_name);
        while (!i.eof()){
            auto creature = new creature_meta_t;
            i >> creature->id;
            i >> creature->name;

            string element_name;
            i >> element_name;
            creature->element = get_element_by_name(element_name);
            creatures_temp->push_back(creature);
        }
        i.close();
        creatures = creatures_temp;
    }

    void load_evolutions(){
        auto* evolutions_temp = new vector<const evolution_meta_t*>;
        std::ifstream i(evolutions_file_name);
        while (!i.eof()){
            auto creature = new evolution_meta_t;

            i >> creature->creature_id;
            i >> creature->level;

            i >> creature->strength;
            i >> creature->max_health;
            i >> creature->agility;

            i >> creature->bounty_exp;
            i >> creature->required_exp;

            i >> creature->skill_type;
            i >> creature->skill_power;

            string evolution_name;
            i >> evolution_name;
            creature->name = evolution_name;

            evolutions_temp->push_back(creature);
        }
        i.close();
        evolutions = evolutions_temp;
    }


    const evolution_meta_t* find_default_evolution_for_creature(const creature_meta_t* creature_metadata) {
        for (auto evolution : *evolutions){
            if(evolution->creature_id != creature_metadata->id) continue;
            if(evolution->level != 0) continue;
            return evolution;
        }
        return nullptr;
    }

    const creature_meta_t* find_random_creature_metadata(){
        auto random_creature_metadata_id = rand() % creatures->size();
        return creatures->at(random_creature_metadata_id);
    }


    /// Loads game metadata from files. Exceptions are not handled.
    void import_data(){
        cout << "Loading difficulties";
        load_difficulties();

        cout << ", m_creatures";
        load_creatures();

        cout << ", evolutions";
        load_evolutions();

        cout << " - OK." << endl;
    }
}



namespace logic{
    using namespace data_model;
    using namespace data_importing;
    using namespace maths2;

    class creature_t : public creature_i{
    private:
        float m_health;
        float m_exp;
        const creature_meta_t* m_creature_meta;
        const evolution_meta_t* m_evolution_meta;

    public:
        /// Creates new instance of given type of creature.
        /// @param health Initial health.
        /// @param creature_metadata Metadata of creature type. (Not disposed)
        /// @param evolution_metadata Metadata of initial creature evolution. (Not disposed)
        creature_t(const creature_meta_t *creature_metadata, const evolution_meta_t *evolution_metadata, float health, float exp) :
                m_health(health), m_creature_meta(creature_metadata), m_evolution_meta(evolution_metadata), m_exp(exp) {}

        float get_health() override { return m_health; }
        float get_exp() override { return m_exp; }
        bool is_alive() override { return m_health > 0; }
        const evolution_meta_t* get_evolution() override { return m_evolution_meta; }
        const creature_meta_t* get_creature() override { return m_creature_meta; }

        void evolute() {
            if(!can_evolute())
            {
                cout << "INTERNAL ERROR: Can not evolute the creature!" << std::endl;
                return;
            }
            m_evolution_meta = m_evolution_meta->next_evolution;

            auto missing_hp = m_evolution_meta->max_health - m_health;
            m_health = m_evolution_meta->max_health - missing_hp / 2.0f;
        }

        void damage_anonymously(float p) {
            m_health -= abs(p);
            m_health = clamp(m_health, 0, get_evolution()->max_health);
        }


        void give_exp(float p) {
            m_exp -= abs(p);
            m_exp = clamp(p, 0, get_evolution()->required_exp);
        }
    };



    class team_t : public team_i{
    private:
        int m_selection_index;
        vector<creature_t*> m_creatures;


    public:
        /// Creates team based on player picks.
        /// @param picks Player picks. (Not disposed.)
        explicit team_t(const vector<const creature_meta_t*>* picks) {
            m_selection_index = 0;
            m_creatures = vector<creature_t*>();
            for (auto pick : *picks) {
                append_team(pick);
            }
        }

        /// Creates team of random creatures and of given size.
        /// @param team_size Number of created creatures.
        explicit team_t(size_t team_size) {
            m_selection_index = 0;
            m_creatures = vector<creature_t*>();
            for (int i = 0; i < team_size; ++i) {
                auto pick = find_random_creature_metadata();
                append_team(pick);
            }
        }

        size_t get_creature_count() override { return m_creatures.size(); }
        creature_i* get_creature(int index) override { return m_creatures.at(index); }
        creature_i* get_selected_creature() override { return m_creatures.at(m_selection_index); }

        creature_t* get_creature_mutable(int index) { return m_creatures.at(index); }
        creature_t* get_selected_creature_mutable() { return m_creatures.at(m_selection_index); }
        void set_selected_creature(int index) { m_selection_index = index; }

        bool is_defeated() override {
            for (auto creature : m_creatures) {
                if(creature->is_alive()) return false;
            }
            return true;
        }

        bool is_creature_selectable(int index) override { return m_creatures.at(index)->is_alive(); }

    private:
        void append_team(const creature_meta_t *pick) {
            auto evolution = find_default_evolution_for_creature(pick);
            if(evolution == nullptr){
                cout << "INTERNAL ERROR: Creature " + pick->name + " has not init evolution!" << endl;
            }
            m_creatures.push_back(new creature_t(pick, evolution, evolution->max_health, 0));
        }
    };



    class game_status_t : public game_status_i{
    private:
        bool m_is_player_turn;
        int m_turn_index;
        team_t* m_player_team;
        vector<team_t*>* m_enemy_teams;

    public:

        int get_turn_index() override { return m_turn_index; }
        bool is_player_turn() override { return m_is_player_turn; }

        size_t get_enemy_teams_count() override{ return m_enemy_teams->size(); }
        team_i* get_player_team() override { return m_player_team; }
        team_i* get_enemy_team(int index) override{ return m_enemy_teams->at(index); }
        team_t* get_player_team_mutable() { return m_player_team; }
        team_t* get_enemy_team_mutable(int index){ return m_enemy_teams->at(index); }

        /// Creates new game based on initial values.
        /// @param player_picks Picks of the player. (Not disposed.)
        /// @param difficulty Difficulty of the game. (Not disposed.)
        game_status_t(
                const vector<const struct creature_meta_t*>* player_picks,
                const difficulty_t* difficulty)
        {
            m_is_player_turn = true;
            m_turn_index = 0;

            m_player_team = new team_t(player_picks);

            m_enemy_teams = new vector<team_t*>();

            for (int i = 0; i < difficulty->enemy_count; ++i) {
                auto new_enemy_team = new team_t(difficulty->player_count + i);
                m_enemy_teams->push_back(new_enemy_team);
            }
        }


        bool can_make_turn_select_any_creature(bool player_team) override{
            int available_creatures = 0;
            for (int i = 0; i < get_team(player_team)->get_creature_count(); ++i) {
                if(can_make_turn_select_creature(player_team, i))
                    available_creatures++;
            }
            return available_creatures > 1;
        }
        bool can_make_turn_select_creature(bool player_team, int selection_index) override {
            return get_team(player_team)->is_creature_selectable(selection_index);
        }
        bool can_make_turn_evolute(bool player_team) override {
            return get_team(player_team)->get_selected_creature()->can_evolute();
        }
        bool can_make_turn_use_attack(bool player_team) override {
            return get_team(player_team)->get_selected_creature()->is_alive();
        }
        bool can_make_turn_use_skill(bool player_team) override {
            return
                    get_team(player_team)->get_selected_creature()->is_alive() &&
                    get_team(player_team)->get_selected_creature()->get_evolution()->skill_type > 0;
        }

        void make_turn_select_creature(bool player_team, int selection_index) override {
            get_enemy_team_mutable(player_team)->set_selected_creature(selection_index);
        }
        void make_turn_evolute(bool player_team) override {
            get_enemy_team_mutable(player_team)->get_selected_creature_mutable()->evolute();
        }
        void make_turn_use_attack(bool player_team) override {
            auto target = get_team(!player_team)->get_selected_creature_mutable();
            auto attacker = get_team(player_team)->get_selected_creature_mutable();

            damage(attacker, target, attacker->get_evolution()->strength);
        }
        void make_turn_use_skill(bool player_team) override {
            auto target = get_team(!player_team)->get_selected_creature_mutable();
            auto attacker = get_team(!player_team)->get_selected_creature_mutable();

            //TODO Skill attack
            damage(attacker, target, attacker->get_evolution()->skill_power);
        }

        void swap_turns() override{ m_is_player_turn = !m_is_player_turn; }

    private:
        team_t* get_team(bool player_team){
            return player_team ? m_player_team : m_enemy_teams->at(0);
        }

        static void damage(creature_t* attacker, creature_t* target, float value){
            target->damage_anonymously(value);

            if(!target->is_alive()){
                attacker->give_exp(target->get_evolution()->bounty_exp);
            }


            // show_creature_damaging(attacker, target, value); //TODO
        }
    };




    game_status_i* start_new_game(const vector<const creature_meta_t*>* player_picks, const difficulty_t* difficulty) {
        return new game_status_t(player_picks, difficulty);
    }
}



namespace ai{
    using namespace data_model;

    player_action get_enemy_action(game_status_i* game_status){
        int randomizer = rand() % 10;

        if(game_status->can_make_turn_use_attack(false) && randomizer > 3)
            return player_action_attack;
        if(game_status->can_make_turn_use_skill(false) && randomizer > 5)
            return player_action_skill_use;
        if(game_status->can_make_turn_evolute(false) && randomizer > 7)
            return player_action_evolution;
        if(game_status->can_make_turn_select_any_creature(false))
            return player_action_creature_reselection;

        return get_enemy_action(game_status);
    }

    int get_enemy_selection(game_status_i* game_status) {
        auto team =
                game_status->get_enemy_team(
                        game_status->get_current_enemy_index());

        int selectable_count = 0;
        for (int i = 0; i < team->get_creature_count(); ++i) {
            if(team->get_selected_creature() == team->get_creature(i)) continue;
            if(team->is_creature_selectable(i)) selectable_count++;
        }

        int selectable_index = 0;
        int selectable_result_index = rand() % selectable_count;
        for (int i = 0; i < team->get_creature_count(); ++i) {
            if(team->get_selected_creature() == team->get_creature(i)) continue;
            if(!team->is_creature_selectable(i)) continue;
            if(selectable_result_index == selectable_index++) return i;
        }

        return -1;
    }
}



namespace view{
    using namespace data_model;


    constexpr float health_display_unit = 5;



    void show_bar(int value, int max, char sign){
        cout << '[';
        for (int j = 0; j < value; ++j) { cout << sign; }
        for (int j = value; j < max; ++j) { cout << ' '; }
        cout << ']';
    }

    void show_bar(float value, float max, float unit, char sign){
        int val_i = (int)(value / unit);
        int max_i = (int)(max / unit);
        show_bar(val_i, max_i, sign);
    }



    void show_select_difficulty_dialog() {
        cout << "Select difficulty:" << endl;
    }

    void show_invalid_index_answer_dialog() {
        cout << "Bruh, there is no answer with such index." << endl;
    }

    void show_selected_option_dialog(const string& selection) {
        cout << "Selected result: " << selection << endl;
    }

    void show_game_start_dialog(int enemy_count) {
        cout
                << "You will face "
                << enemy_count
                << " enemies. Good luck!"
                << endl;
    }

    void show_selectable(int index, const string& value){
        cout << (index) << ") " << value << endl;
    }

    void show_select_team_dialog(int team_size) {
        cout
                << "You need to create a team. Select "
                << team_size << " creatures."
                << endl;
    }

    void show_done_dialog() {
        cout << "Done!" << endl;
    }

    void show_team_presentation_dialog(vector<const creature_meta_t *> *&team) {
        cout << team->at(0)->name;
        for (int i = 1; i < team->size(); ++i) {
            cout << ((i != team->size() - 1) ? ", " : " and ");
            cout << team->at(i)->name;
        }
        cout << " will be a great team!" << endl;
    }

    void show_team_status2(team_i* team, bool player_team) {
            cout << (player_team ? "---* YOUR TEAM *---" : "---* ENEMY TEAM *---") << endl << endl;

        for (int i = 0; i < team->get_creature_count(); ++i) {
            auto creature = team->get_creature(i);

            cout << i << ") "
                << creature->get_creature()->name
                << " <" << (creature->get_evolution()->level + 1) << " '"
                << creature->get_evolution()->name << "'>";

            if(team->get_selected_creature() == creature)
                cout << " (ON ARENA)";

            cout << endl;

            cout << creature->get_health() << "/" << creature->get_evolution()->max_health << " HP ";
            show_bar(creature->get_health(), creature->get_evolution()->max_health, health_display_unit, '=');
            cout << endl;

            cout << creature->get_exp() << "/" << creature->get_evolution()->required_exp << " EXP ";
            show_bar(creature->get_exp(), creature->get_evolution()->required_exp, health_display_unit, '*');
            cout << endl << endl;
        }
    }

    void show_turn(bool player_turn) {
        cout << (player_turn ? "Player turn!" : "Computer turn!") << endl;
    }

    void show_game_start_prompt() {
        cout << "Game is about to start...";
        cout << endl << endl << endl;
    }

    void show_creature_damaging(creature_i *attacker, creature_i *target, float value) {
        cout
                << attacker->get_creature()->name << " has attacked "
                << target->get_evolution()->name << " for "
                << value << "damage." << endl;
    }
}



namespace controller{
    using namespace data_model;
    using namespace data_importing;
    using namespace view;
    using namespace logic;
    using difficulty_cp = const difficulty_t*;
    using team_picks_cp = const vector<const creature_meta_t*>*;

    template<typename t> t keep_asking(function<t()> ask_func);

    constexpr char attack_input = 'a';
    constexpr char skill_input = 's';
    constexpr char evolution_input = 'e';
    constexpr char change_input = 'c';

    player_action ask_for_player_action(game_status_i* game_status) {
        if(game_status->can_make_turn_use_attack(true))
            cout << attack_input<<  ") Use attack" << endl;
        if(game_status->can_make_turn_use_skill(true))
            cout << skill_input << ") Use skill" << endl;
        if(game_status->can_make_turn_evolute(true))
            cout << evolution_input << ") Use attack" << endl;
        if(game_status->can_make_turn_select_any_creature(true))
            cout << change_input << ") Change creature on the arena" << endl;

        char input;
        player_action result = player_action_none;

        while (result == player_action_none)
        {
            cin >> input;
            switch (input) {
                case attack_input: result = player_action_attack; break;
                case skill_input: result = player_action_skill_use; break;
                case evolution_input: result = player_action_evolution; break;
                case change_input: result = player_action_creature_reselection; break;
                default: {
                    show_invalid_index_answer_dialog();
                    result = player_action_none;
                } break;
            }
        }

        return result;
    }

    int ask_for_creature_reselection(team_i* team, bool full) {
        cout << "Select creature sent to the arena:" << endl;

        if(full){
            for (int i = 0; i < team->get_creature_count(); ++i) {
                auto creature = team->get_creature(i);
                cout << i << ") " << creature->get_creature()->name
                     << " <" << creature->get_evolution()->name << ">" << endl;
            }
        }

        int result = -1;
        while (result == -1){
            int input = -1;
            cin >> input;

            if(team->is_creature_selectable(input))
                result = input;
        }
        cout << "The " << team->get_creature(result)->get_creature()->name << " on its way!" << endl;
        return result;
    }

    /// Keeps invoking given asking method until resultant pointer is not null.
    /// @tparam t Type of returned pointer.
    /// @param ask_func Unreliable function that may return null.
    /// @return Never-null pointer to the result.
    template<typename t>
    t keep_asking(function<t()> ask_func) {
        t result = nullptr;
        while (result == nullptr){
            result = ask_func();
        }
        return result;
    }

    /// Asks the player to select difficulty for a new game.
    /// @return Null when invalid answer was selected.
    difficulty_cp ask_for_difficulty() {
        auto difficulties = data_importing::difficulties;
        show_select_difficulty_dialog();
        for (int i = 0; i < difficulties->size(); ++i) {
            auto difficulty = difficulties->at(i);
            show_selectable(i, difficulty->name);
        }
        int difficulty_index = 0;
        cin >> difficulty_index;

        if(difficulty_index < 0 || difficulty_index >= difficulties->size()){
            show_invalid_index_answer_dialog();
            return nullptr;
        }

        show_selected_option_dialog(difficulties->at(difficulty_index)->name);

        difficulty_cp result = difficulties->at(difficulty_index);
        show_game_start_dialog(result->enemy_count);

        return result;
    }

    /// Asks the player to select given number of playable creatures.
    /// @param team_size Number of creatures to build a team.
    /// @return Null when given answer was invalid.
    team_picks_cp ask_for_team(int team_size) {
        show_select_team_dialog(team_size);

        auto creature_types = data_importing::creatures;
        auto creature_types_count = creature_types->size();

        for (int i = 0; i < creature_types_count; ++i) {
            auto creature = creature_types->at(i);
            show_selectable(i, creature->name);
        }

        auto team = new vector<const creature_meta_t*>;
        for (int i = 0; i < team_size; ++i) {
            int array_id = 0;
            cin >> array_id;

            if(array_id < 0 || array_id >= creature_types_count)
            {
                show_invalid_index_answer_dialog();
                return nullptr;
            }

            team->push_back(creature_types->at(array_id));
        }

        show_done_dialog();
        show_team_presentation_dialog(team);

        return team;
    }

    /// Asks player for settings required to create a new game and creates new game.
    /// @return New game instance.
    game_status_i* init_new_game() {
        function < difficulty_cp() > ask_for_difficulty_f(ask_for_difficulty);
        difficulty_cp difficulty = keep_asking(ask_for_difficulty_f);

        int player_team_size = difficulty->player_count;

        function < team_picks_cp() > ask_for_team_f(
                [player_team_size]()->team_picks_cp{return ask_for_team(player_team_size);}
        );
        team_picks_cp player_team = keep_asking(ask_for_team_f);

        return start_new_game(player_team, difficulty);
    }
}




int main() {
    using namespace data_model;
    using namespace data_importing;
    using namespace logic;
    using namespace view;
    using namespace controller;
    using namespace ai;


    import_data();

    auto game = init_new_game();
    show_game_start_prompt();

    show_team_status2(game->get_enemy_team(0), false);
    show_team_status2(game->get_player_team(), true);

    int first_selected_creature = ask_for_creature_reselection(game->get_player_team(), false);
    game->make_turn_select_creature(true, first_selected_creature);


    do{
        bool player_team = game->is_player_turn();
        show_turn(game->is_player_turn());

        player_action player_action;

        if(game->is_player_turn()){
            player_action = ask_for_player_action(game);
        }else{
            player_action = get_enemy_action(game);
        }

        switch (player_action) {
            case player_action_attack: game->make_turn_use_attack(player_team); break;
            case player_action_skill_use: game->make_turn_use_skill(player_team); break;
            case player_action_evolution: game->make_turn_evolute(player_team); break;
            case player_action_creature_reselection: {
                int selection = player_team ?
                                ask_for_creature_reselection(game->get_player_team(), false) :
                                get_enemy_selection(game);
                game->can_make_turn_select_creature(player_team,selection);
            } break;
            case player_action_none:
                cout << "INTERNAL ERROR: Null player action handling." << endl;
                break;
        }

        show_team_status2(game->get_enemy_team(0), false);
        show_team_status2(game->get_player_team(), true);

        game->swap_turns();
    } while (!game->is_game_over());
}