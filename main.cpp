#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <functional>
#include <random>

using std::string;
using std::cout;
using std::cin;
using std::endl;
using std::ifstream;
using std::ofstream;
using std::vector;
using std::function;



namespace maths2{
    /// Limits value to specific bounds
    /// @param t Original value.
    /// @param min Minimal output.
    /// @param max Maximal output.
    /// @return Bounded t.
    float clamp(float t, float min, float max){
        if(t > max) return max;
        if(t < min) return min;
        return t;
    }

    /// Prepares a number to be displayed to user.
    /// @param x Original value.
    /// @return Human-friendly value.
    float display_float(float x){
        if(x <= 0.1f && x > 0.0f) return 0.1f;
        return round(x * 10) / 10;
    }
}



namespace rng{
    using std::random_device;
    using std::default_random_engine;
    using std::uniform_real_distribution;
    using std::uniform_int_distribution;

    namespace internal{
        random_device* rd2;
        default_random_engine* random_engine;
        uniform_real_distribution<float>* default_distribution;
    }

    using namespace rng::internal;

    void init_module_rng(){
        rd2 = new random_device();
        random_engine = new default_random_engine((*rd2)());
        default_distribution = new uniform_real_distribution(0.0f, 1.0f);

        cout << "RNG initialized." << endl;
    }

    float next_random_float_01(){
        return default_distribution->operator()(*random_engine);
        // return (*default_distribution)(*random_engine);
    }

    int next_random_index(size_t len){
        int len2 = static_cast<size_t>(len);
        uniform_int_distribution distribution(0, len2 - 1);
        return distribution(*internal::random_engine);
    }
}



namespace events{
    template<class args_t>
    class event{
    private:
        vector<function<void(args_t)>> listeners;

    public:
        /// Adds function to invoke list making it a listener.
        /// @param listener New listener.
        void subscribe(function<void(args_t)> listener){
            listeners.push_back(listener);
        }

        /// Invokes all listeners.
        /// @param args Argument passed to all listeners.
        void invoke(args_t args){
            for (const auto& listener : listeners) {
                listener(args);
            }
        }
    };
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

        const evolution_meta_t* next_evolution;
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
        int get_selectable_creature_count();

        virtual bool is_defeated() = 0;
    };

    int team_i::get_selectable_creature_count() {
        int result = 0;
        for (int i = 0; i < get_creature_count(); ++i) {
            if(get_creature(i)->is_alive()) result++;
        }
        return result;
    }

    class game_status_i{
    public:
        virtual int get_turn_index() = 0;
        virtual bool is_player_turn() = 0;
        virtual team_i* get_player_team() = 0;
        virtual size_t get_enemy_teams_count() = 0;
        virtual team_i* get_enemy_team(int index) = 0;
        virtual int get_current_enemy_index() = 0;

        bool are_all_enemy_teams_defeated();
        bool is_round_over();
        bool is_game_over();
        team_i* get_current_enemy_team();

        virtual bool can_make_turn_select_any_creature(bool player_team) = 0;
        virtual bool can_make_turn_select_creature(bool player_team, int selection_index) = 0;
        virtual bool can_make_turn_evolute(bool player_team) = 0;
        virtual bool can_make_turn_use_attack(bool player_team) = 0;
        virtual bool can_make_turn_use_skill(bool player_team) = 0;

        virtual void make_turn_select_creature(bool player_team, int selection_index) = 0;
        virtual void make_turn_evolute(bool player_team) = 0;
        virtual void make_turn_use_attack(bool player_team) = 0;
        virtual void make_turn_use_skill(bool player_team) = 0;

        virtual bool try_make_obligatory_turn(bool player_team) = 0;
        virtual void swap_turns() = 0;
        virtual bool try_fight_next_enemy() = 0;

        virtual ~game_status_i() { }
    };



    bool game_status_i::are_all_enemy_teams_defeated() {
        for (int i = 0; i < get_enemy_teams_count(); ++i) {
            auto enemy_team = get_enemy_team(i);
            if(!enemy_team->is_defeated())
                return false;
        }
        return true;
    }

    bool game_status_i::is_round_over() {
        bool player_team_defeated = get_player_team()->is_defeated();
        bool enemy_team_defeated = get_current_enemy_team()->is_defeated();
        return player_team_defeated || enemy_team_defeated;
    }

    bool game_status_i::is_game_over() {
        return get_player_team()->is_defeated() || are_all_enemy_teams_defeated();
    }

    team_i *game_status_i::get_current_enemy_team() {
        return get_enemy_team(get_current_enemy_index());
    }

    bool creature_i::can_evolute() {
        return
                this->get_exp() >= this->get_evolution()->required_exp &&
                this->get_evolution()->next_evolution != nullptr &&
                this->is_alive();
    }



    struct damage_i{
        creature_i* attacker;
        creature_i* target;
        float value;
    };

    struct selection_i{
        int index;
        creature_i* selected;
        bool is_player_team;
    };


    struct element_interaction_i{
        const element attacker;
        const element target;
        const float multiplier;
    };
}



namespace data_importing{
    using namespace data_model;

    const vector<const difficulty_t*>* difficulties;
    const vector<const creature_meta_t*>* creatures;
    const vector<const evolution_meta_t*>* evolutions;
    const vector<const element_interaction_i*>* element_interactions;

    const char* difficulties_file_name = "Difficulties.txt";
    const char* evolutions_file_name = "Evolutions.txt";
    const char* creatures_file_name = "Creatures.txt";
    constexpr float element_interaction_damage_mul_buff = 1.5f;
    constexpr float element_interaction_damage_mul_nerf = 1.0f / element_interaction_damage_mul_buff;

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

    float find_element_damage_mul(element attacker, element target){
        for(auto element_interaction : *element_interactions){
            if(attacker != element_interaction->attacker) continue;
            if(target != element_interaction->target) continue;
            return element_interaction->multiplier;
        }
        return 1.0f;
    }


    namespace internal
    {
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


        const evolution_meta_t* find_next_evolution(const vector<const evolution_meta_t*>* src, const evolution_meta_t* base){
            for(auto evolution : *src){
                if(evolution->creature_id != base->creature_id) continue;
                if(evolution->level != base->level + 1) continue;
                return evolution;
            }
            return nullptr;
        }

        void load_evolutions(){
            auto* evolutions_temp = new vector<const evolution_meta_t*>;
            std::ifstream i(evolutions_file_name);
            while (!i.eof()){
                auto evolution = new evolution_meta_t;

                i >> evolution->creature_id;
                i >> evolution->level;

                i >> evolution->strength;
                i >> evolution->max_health;
                i >> evolution->agility;

                i >> evolution->bounty_exp;
                i >> evolution->required_exp;

                i >> evolution->skill_type;
                i >> evolution->skill_power;

                string evolution_name;
                i >> evolution_name;
                evolution->name = evolution_name;

                evolution->next_evolution = find_next_evolution(evolutions_temp, evolution);

                evolutions_temp->push_back(evolution);
            }
            i.close();
            evolutions = evolutions_temp;
        }


        void load_element_interactions(){
            auto nerf = element_interaction_damage_mul_nerf;
            auto buff = element_interaction_damage_mul_buff;

            element_interactions = new vector<const element_interaction_i*>{
                new element_interaction_i{ water, water, nerf },
                new element_interaction_i{ water, earth, buff },
                new element_interaction_i{ water, fire, buff },

                new element_interaction_i{ earth, air, nerf },
                new element_interaction_i{ earth, fire, buff },
                new element_interaction_i{ earth, ice, buff },
                new element_interaction_i{ earth, metal, buff},

                new element_interaction_i{ air, earth, nerf },
                new element_interaction_i{ air, ice, buff },
                new element_interaction_i{ air, metal, buff },

                new element_interaction_i{ fire, water, nerf },
                new element_interaction_i{ fire, earth, buff },
                new element_interaction_i{ fire, ice, buff },
                new element_interaction_i{ fire, metal, nerf },

                new element_interaction_i{ ice, water, nerf },
                new element_interaction_i{ ice, earth, buff },
                new element_interaction_i{ ice, fire, nerf },
                new element_interaction_i{ ice, ice, nerf },

                new element_interaction_i{ metal, water, buff },
                new element_interaction_i{ metal, air, buff },
                new element_interaction_i{ metal, fire, nerf },
                new element_interaction_i{ metal, metal, nerf },
            };
        }
    }
    using namespace data_importing::internal;

    /// Loads game metadata from files or hard-coded data. Exceptions are not handled.
    void init_module_importing_data(){
        cout << "Loading difficulties";
        load_difficulties();

        cout << ", creatures";
        load_creatures();

        cout << ", evolutions";
        load_evolutions();

        cout << ", element interactions";
        load_element_interactions();

        cout << " - OK." << endl;
    }
}



namespace logic{
    using namespace data_model;
    using namespace data_importing;
    using namespace maths2;
    using namespace rng;
    using namespace events;

    /// Event invoked after damaging a creature by a different creature.
    event<damage_i> on_damage;
    /// Event invoked after dealing enough damage_default_attack to declare a creature dead.
    event<creature_i*> on_death;
    /// Event invoked after a selection.
    event<selection_i> on_selection;
    /// Event invoked after an evolution.
    event<creature_i*> on_evolution;
    /// Event invoked whenever some turn is forced.
    event<player_action> on_obligatory_turn;
    /// Event invoked before passing defeated enemy.
    event<int> on_enemy_pass;


    namespace internal
    {
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
                m_exp += abs(p);
                m_exp = clamp(p, 0, get_evolution()->required_exp);
            }

            void heal_full(){
                m_health = get_evolution()->max_health;
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
            void set_selected_creature(int index) {
                m_selection_index = index;
            }

            bool is_defeated() override {
                for (auto creature : m_creatures) {
                    if(creature->is_alive()) return false;
                }
                return true;
            }

            bool is_creature_selectable(int index) override { return m_creatures.at(index)->is_alive(); }

            ~team_t(){
                for (auto creature : m_creatures) {
                    delete creature;
                }
            }

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
            int m_enemy_index;
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
            int get_current_enemy_index() override { return m_enemy_index; }

            /// Creates new game based on initial values.
            /// @param player_picks Picks of the player. (Not disposed.)
            /// @param difficulty Difficulty of the game. (Not disposed.)
            game_status_t(
                    const vector<const struct creature_meta_t*>* player_picks,
                    const difficulty_t* difficulty)
            {
                m_is_player_turn = true;
                m_turn_index = 0;
                m_enemy_index = 0;

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
                return available_creatures > 0;
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
                get_team(player_team)->set_selected_creature(selection_index);
                on_selection.invoke({selection_index, get_team(player_team)->get_selected_creature(), player_team});
                m_turn_index++;
            }
            void make_turn_evolute(bool player_team) override {
                creature_t* creature = get_team(player_team)->get_selected_creature_mutable();
                creature->evolute();
                on_evolution.invoke(creature);
                m_turn_index++;
            }
            void make_turn_use_attack(bool player_team) override {
                auto target = get_team(!player_team)->get_selected_creature_mutable();
                auto attacker = get_team(player_team)->get_selected_creature_mutable();

                damage_default_attack(attacker, target);
                m_turn_index++;
            }
            void make_turn_use_skill(bool player_team) override {
                auto target = get_team(!player_team)->get_selected_creature_mutable();
                auto attacker = get_team(player_team)->get_selected_creature_mutable();

                //TODO Skill attack
                damage_default_attack(attacker, target);
                m_turn_index++;
            }


            bool try_make_obligatory_turn(bool player_team) override{
                auto team = get_team(player_team);

                if(!team->get_selected_creature()->is_alive() &&
                    team->get_selectable_creature_count() == 1){
                    for (int i = 0; i < team->get_creature_count(); ++i) {
                        if(team->is_creature_selectable(i)) {
                            make_turn_select_creature(player_team, i);
                            on_obligatory_turn.invoke(player_action_creature_reselection);
                            return true;
                        }
                    }
                }

                return false;
            }

            void swap_turns() override { m_is_player_turn = !m_is_player_turn; }

            bool try_fight_next_enemy() override{
                if(!get_current_enemy_team()->is_defeated())
                    throw std::invalid_argument("Fight with next enemy team requires defeating contemporary one.");

                if(get_current_enemy_index() == get_enemy_teams_count() - 1)
                    return false;

                on_enemy_pass.invoke(m_enemy_index);
                m_enemy_index++;

                {
                    auto player_team = get_player_team_mutable();
                    for (int i = 0; i < player_team->get_creature_count(); ++i) {
                        auto creature = player_team->get_creature_mutable(i);
                        creature->heal_full();
                        creature->give_exp(5);
                    }
                }

                return true;
            }

            ~game_status_t() override{
                for (auto &m_enemy_team : *m_enemy_teams) {
                    delete m_enemy_team;
                }

                delete m_enemy_teams;
                delete m_player_team;

                cout << "Disposing game - OK." << endl;
            }

        private:
            /// Gets contemporary team involved in fight.
            /// @param player_team Informs if the player's team is mentioned.
            /// @return Pointer to mutable fighting team.
            team_t* get_team(bool player_team){
                return player_team ? m_player_team : m_enemy_teams->at(m_enemy_index);
            }

            static void damage_default_attack(creature_t* attacker, creature_t* target){
                const float power = attacker->get_evolution()->strength;
                const float element_mul = find_element_damage_mul(
                        attacker->get_creature()->element,
                        target->get_creature()->element);

                float result_dmg = power * element_mul;

                const float miss_possibility = 1 - (target->get_evolution()->agility / 100.0f);
                if(rng::next_random_float_01() > miss_possibility)
                    result_dmg = 0;

                target->damage_anonymously(result_dmg);
                on_damage.invoke({attacker, target, result_dmg});

                if(!target->is_alive()){
                    attacker->give_exp(target->get_evolution()->bounty_exp);
                    on_death.invoke(target);
                }
            }
        };
    }
    using namespace logic::internal;


    game_status_i* start_new_game(const vector<const creature_meta_t*>* player_picks, const difficulty_t* difficulty) {
        return new game_status_t(player_picks, difficulty);
    }

    game_status_i* open_game(const string& save_name){
        //TODO Opening game.
        return nullptr;
    }

    void save_game(const string& save_name, const game_status_i* game_status){
        //TODO Saving game.
        cout << save_name << " saved. Unfortunatelly it's only mock :<" << endl;
    }
}



namespace ai{
    using namespace data_model;

    player_action get_enemy_action(game_status_i* game_status){
        vector<player_action> results;

        if(game_status->can_make_turn_use_attack(false)){
            results.push_back(player_action_attack);
            results.push_back(player_action_attack);
            results.push_back(player_action_attack);
        }
        if(game_status->can_make_turn_use_skill(false)){
            results.push_back(player_action_skill_use);
            results.push_back(player_action_skill_use);
        }
        if(game_status->can_make_turn_evolute(false)){
            results.push_back(player_action_evolution);
            results.push_back(player_action_evolution);
            results.push_back(player_action_evolution);
            results.push_back(player_action_evolution);
            results.push_back(player_action_evolution);
        }
        if(game_status->get_current_enemy_team()->get_selectable_creature_count() > 1){
            results.push_back(player_action_creature_reselection);
        }

        int random_index = rng::next_random_index(results.size());
        return results.at(random_index);
    }

    int get_enemy_selection(game_status_i* game_status) {
        auto team =
                game_status->get_enemy_team(
                        game_status->get_current_enemy_index());

        vector<int> selectables;

        for (int i = 0; i < team->get_creature_count(); ++i) {
            if(team->get_selected_creature() == team->get_creature(i))
                continue;
            if(team->is_creature_selectable(i))
                selectables.push_back(i);
        }

        if(selectables.empty())
            throw std::invalid_argument("Selectables it empty!");

        int random_index = rng::next_random_index(selectables.size());
        return selectables.at(random_index);
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

    void show_not_yet_implemented_dialog(){
        cout << "Not yet implemented." << endl;
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
        cout << endl<< (player_team ? "---* YOUR TEAM *---" : "---* ENEMY TEAM *---") << endl;

        for (int i = 0; i < team->get_creature_count(); ++i) {
            auto creature = team->get_creature(i);

            cout << i << ") "
                << creature->get_creature()->name
                << " <" << (creature->get_evolution()->level + 1) << " '"
                << creature->get_evolution()->name << "'>";

            if(team->get_selected_creature() == creature)
                cout << " <--- ON ARENA --->";

            cout << endl;


            if(!team->get_creature(i)->is_alive()) {
                cout << "-- DEAD -- ";
                cout << "\t\t";
            }
            else{
                cout << maths2::display_float(creature->get_health()) << "/" << maths2::display_float(creature->get_evolution()->max_health) << " HP ";
                show_bar(creature->get_health(), creature->get_evolution()->max_health, health_display_unit, '=');
                cout << "\t\t\t\t";
            }

            cout << maths2::display_float(creature->get_exp()) << "/" << maths2::display_float(creature->get_evolution()->required_exp) << " EXP ";
            show_bar(creature->get_exp(), creature->get_evolution()->required_exp, health_display_unit, '*');
            cout << endl;
        }
    }

    void show_turn(bool player_turn) {
        cout << endl << (player_turn ? "---* PLAYER TURN *---" : "---* COMPUTER TURN *---") << endl;
    }

    void show_game_start_prompt() {
        cout << "Game is about to start...";
        cout << endl << endl << endl;
    }

    void show_creature_damaging(const damage_i& dmg_i) {
        if(dmg_i.value > 0)
            cout
                << dmg_i.attacker->get_creature()->name << " has attacked "
                << dmg_i.target->get_creature()->name << " for "
                << maths2::display_float(dmg_i.value) << " HP." << endl;
        else
            cout
                << dmg_i.target->get_creature()->name <<  " has dodged the attack of "
                << dmg_i.attacker->get_creature()->name << ". (Probability "
                << dmg_i.target->get_evolution()->agility << "%)" << endl;
    }

    void show_creature_death(creature_i* corpse){
        cout << corpse->get_creature()->name << " has died!" << endl;
    }

    void show_selection(selection_i selection){
        cout
        << (selection.is_player_team ? "Player" : "Bot" )
        << " has selected " << selection.selected->get_creature()->name
        << " (" << selection.index << ")" << endl;
    }

    void show_evolution(creature_i* creature){
        cout << creature->get_creature()->name << " has evolved into " << creature->get_evolution()->name << endl;
    }

    void show_round_winner(bool player_team){
        cout << endl << "--*-- --*-- --*--" << endl;

        if(player_team){
            cout << "PLAYER WINS ROUND!" << endl;
        }else{
            cout << "COMPUTER WINS ROUND!" << endl;
        }

        cout << "--*-- --*-- --*--" << endl;
        cout << endl;
    }

    void show_game_winner(bool player_team){
        cout << endl << "==*== ==*== ==*==" << endl;

        if(player_team){
            cout << "PLAYER WINS GAME!" << endl;
        }else{
            cout << "COMPUTER WINS GAME!" << endl;
        }

        cout << "==*== ==*== ==*==" << endl;
        cout << endl;
    }

    void show_main_menu(){
        cout << "===[]==[ MAIN MENU ]==[]===" << endl;
        cout << "0) New game" << endl;
        cout << "1) Load game" << endl;
        cout << "2) Exit" << endl;
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

    constexpr char attack_input_key = 'a';
    constexpr char skill_input_key = 's';
    constexpr char evolution_input_key = 'e';
    constexpr char change_input_key = 'c';

    /// Asks the player what type of action he wants his creature on arena to perform.
    /// @param game_status Contemporary game status. //TODO This method is too privileged.
    /// @return Selected player action.
    player_action ask_for_player_action(game_status_i* game_status) {
        if(game_status->can_make_turn_use_attack(true))
            cout << attack_input_key << ") Use attack" << endl;
        if(game_status->can_make_turn_use_skill(true))
            cout << skill_input_key << ") Use skill" << endl;
        if(game_status->can_make_turn_evolute(true))
            cout << evolution_input_key << ") Evolution" << endl;
        if(game_status->can_make_turn_select_any_creature(true))
            cout << change_input_key << ") Change creature on the arena" << endl;

        char input;
        player_action result = player_action_none;

        while (result == player_action_none)
        {
            cin >> input;
            switch (input) {
                case attack_input_key: result = player_action_attack; break;
                case skill_input_key: result = player_action_skill_use; break;
                case evolution_input_key: result = player_action_evolution; break;
                case change_input_key: result = player_action_creature_reselection; break;
                default: {
                    show_invalid_index_answer_dialog();
                    result = player_action_none;
                } break;
            }

            if(
                (result == player_action_attack && !game_status->can_make_turn_use_attack(true)) ||
                (result == player_action_skill_use && !game_status->can_make_turn_use_skill(true)) ||
                (result == player_action_evolution && !game_status->can_make_turn_evolute(true)) ||
                (result == player_action_creature_reselection && !game_status->can_make_turn_select_any_creature(true))
            ) {
                result = player_action_none;
                show_invalid_index_answer_dialog();
            }
        }

        return result;
    }

    /// Asks the player to select different creature to be fighting.
    /// @param team The team of player.
    /// @return Index of newly selected creature.
    int ask_for_creature_reselection(team_i* team) {
        cout << "Select creature sent to the arena:" << endl;

        int result = -1;
        while (result == -1){
            int input = -1;
            cin >> input;

            if(input >= team->get_creature_count() || input < 0)
                show_invalid_index_answer_dialog();
            else if(team->is_creature_selectable(input))
                result = input;
            else
                show_invalid_index_answer_dialog();
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

    /// Asks player if saving is required and potentially performs it.
    /// @param saving_func
    void ask_for_saving(const function<void(const string&, game_status_i*)>& saving_func, game_status_i* game_status){
        cout << "Do you want to save the game? (y/n)" << endl;

        bool save;

        char response = '\0';
        while (response == '\0'){
            cin >> response;
            switch (response) {
                case 'y':{
                    save = true;
                }break;

                case 'n':{
                    save = false;
                }break;

                default:{
                    response = '\0';
                    cout << "Invalid input!" << endl;
                }break;
            }
        }

        if(!save) return;

        cout << "Enter save name:" << endl;

        string save_name;
        cin >> save_name;

        saving_func(save_name, game_status);
    }
}


using namespace data_model;
using namespace data_importing;
using namespace logic;
using namespace view;
using namespace controller;
using namespace ai;
using namespace rng;


void static_init_modules();
void main_menu();
void play(game_status_i* game);


int main() {
    static_init_modules();

    main_menu();
}


void main_menu() {
    bool exit = false;

    show_main_menu();
    int input = -1;

    while (!exit){
        cin >> input;

        switch (input) {
            case 0: {
                auto game = init_new_game();
                play(game);
                delete game;

                show_main_menu();
            }
            break;

            case 1: {
                show_not_yet_implemented_dialog();
            }break;

            case 2: {
                exit = true;
            }break;

            default: {
                show_invalid_index_answer_dialog();
            }break;
        }
    }
}


void static_init_modules() {
    on_damage.subscribe(show_creature_damaging);
    on_death.subscribe(show_creature_death);
    on_selection.subscribe(show_selection);
    on_evolution.subscribe(show_evolution);
    on_obligatory_turn.subscribe([=](player_action){
        cout << "(Obligatory turn)" << endl;
    });
    on_enemy_pass.subscribe([=](int enemy_index){
        cout << "--- *** --- *** ---" << endl;
        cout << "ENEMY No." << enemy_index << " DEFEATED!!!" << endl;
        cout << "--- *** --- *** ---" << endl << endl;
    });

    init_module_rng();
    init_module_importing_data();
}


void play(game_status_i* game) {
    show_game_start_prompt();
    show_team_status2(game->get_player_team(), true);

    int first_selected_creature = ask_for_creature_reselection(game->get_player_team());
    game->make_turn_select_creature(true, first_selected_creature);


    while (!game->is_game_over()){
        do{
            bool player_team = game->is_player_turn();

            if(game->is_player_turn())
            {
                show_team_status2(game->get_current_enemy_team(), false);
                show_team_status2(game->get_player_team(), true);
            }

            show_turn(game->is_player_turn());

            if(!game->try_make_obligatory_turn(player_team))
            {
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
                            ask_for_creature_reselection(game->get_player_team()) :
                            get_enemy_selection(game);
                        game->make_turn_select_creature(player_team,selection);
                    } break;
                }
            }

            game->swap_turns();
        }
        while (!game->is_round_over());

        if(game->get_player_team()->is_defeated()){
            show_round_winner(false);
        }else{
            show_round_winner(true);
            bool exists_next_enemy = game->try_fight_next_enemy();

            if(!exists_next_enemy)
                break;
            else
                ask_for_saving(save_game, game);
        }
    }
    show_game_winner(!game->get_player_team()->is_defeated());
}