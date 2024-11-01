#include "MealyMooreConverter.h"

using namespace std;
string inputFile = "1_moore.csv";
string secondFile = "1_mealy.csv";
string outputFile = "output.csv";
const string MOORE_STATE_CH = "q";

struct pair_hash {
    template <class T1, class T2>
    std::size_t operator () (const std::pair<T1, T2>& pair) const {
        auto hash1 = std::hash<T1>{}(pair.first);
        auto hash2 = std::hash<T2>{}(pair.second);
        return hash1 ^ hash2;
    }
};

struct MooreAutomata {
    unordered_map<string, string> outputs;
    vector<vector<string>> transitions;  
    unordered_set<string> states;
    unordered_set<string> inputs;
};

struct MealyAutomata {
    vector<vector<pair<string, string>>> transitions;
    unordered_set<string> states;
    unordered_set<string> inputs;
};

MooreAutomata ReadMoore(string& input_file) {
    MooreAutomata aut;
    ifstream file(input_file);
    string line;
    if (!file.is_open()) 
    {
        cerr << "Error: Could not open file " << input_file << endl;
        return aut;
    }
    // Чтение выходных символов
    getline(file, line);
    stringstream ss_outputs(line);
    string output;
    getline(ss_outputs, output, ';'); // Пропуск первого столбца
    vector<string> output_symbols;
    while (getline(ss_outputs, output, ';')) 
    {
        output_symbols.push_back(output);
    }
    // Чтение имен состояний
    getline(file, line);
    stringstream ss_states(line);
    string state;
    getline(ss_states, state, ';'); // Пропуск первого столбца
    int stateIndex = 0;
    while (getline(ss_states, state, ';')) 
    {
        aut.states.insert(state);
        aut.outputs[state] = output_symbols[stateIndex];
        stateIndex++;
    }
    // Чтение переходов
    while (getline(file, line)) 
    {
        stringstream ss(line);
        string input, transition;
        getline(ss, input, ';');
        aut.inputs.insert(input);
        vector<string> state_transitions;
        while (getline(ss, transition, ';')) 
        {
            state_transitions.push_back(transition);
        }
        aut.transitions.push_back(state_transitions);
    }
    return aut;
}

MealyAutomata ReadMealy(const string& input_file) {
    MealyAutomata mealyAutomata;
    ifstream file(input_file);
    if (!file.is_open()) {
        cerr << "Error: Could not open file " << input_file << endl;
        return mealyAutomata;
    }
    string line;
    // Чтение заголовка (состояния)
    getline(file, line);
    stringstream ss_states(line);
    string state;
    getline(ss_states, state, ';'); // Пропуск первого столбца
    while (getline(ss_states, state, ';')) {
        mealyAutomata.states.insert(state);
    }
    // Чтение переходов
    while (getline(file, line)) {
        stringstream ss(line);
        string input, transition;
        getline(ss, input, ';');
        mealyAutomata.inputs.insert(input);
        vector<pair<string, string>> mealyTransitions;
        while (getline(ss, transition, ';')) {
            string nextState = transition.substr(0, transition.find('/'));
            string output = transition.substr(transition.find('/') + 1);
            mealyTransitions.push_back({ nextState, output });
        }
        mealyAutomata.transitions.push_back(mealyTransitions);
    }
    return mealyAutomata;
}

void PrintMooreAutomata(const MooreAutomata automata) {
    cout << "Outputs:" << endl;
    for (const auto& pair : automata.outputs) {
        cout << pair.first << " -> " << pair.second << endl;
    }

    cout << "\nStates:" << endl;
    for (const auto& state : automata.states) {
        cout << state << endl;
    }

    cout << "\nInputs:" << endl;
    for (const auto& input : automata.inputs) {
        cout << input << endl;
    }

    cout << "\nTransitions:" << endl;
    for (size_t i = 0; i < automata.transitions.size(); i++) {
        cout << "Input: " << *next(automata.inputs.begin(), i) << "\n";
        for (size_t j = 0; j < automata.transitions[i].size(); j++) {
            cout << "State: " << *next(automata.states.begin(), j) << " -> " << automata.transitions[i][j] << "\n";
        }
    }
}

void PrintMealyAutomata(const MealyAutomata mealyAutomata) {
    cout << "States:" << endl;
    for (const auto& state : mealyAutomata.states) {
        cout << state << endl;
    }

    cout << "\nInputs:" << endl;
    for (const auto& input : mealyAutomata.inputs) {
        cout << input << endl;
    }

    cout << "\nTransitions:" << endl;
    for (size_t i = 0; i < mealyAutomata.transitions.size(); ++i) {
        cout << "Input: " << *next(mealyAutomata.inputs.begin(), i) << endl;
        for (size_t j = 0; j < mealyAutomata.transitions[i].size(); ++j) {
            cout << "State: " << *next(mealyAutomata.states.begin(), j) << " -> " << mealyAutomata.transitions[i][j].first << "/" << mealyAutomata.transitions[i][j].second << endl;
        }
    }
}

void exportMooreToCSV(MooreAutomata automata, const string& filename) {
    ofstream file(filename);
    if (!file.is_open()) {
        cerr << "Failed to open file: " << filename << endl;
        return;
    }
    // outputs
    for (const auto& output : automata.outputs) {
        file << ";" << output.second;
    }
    file << endl;
    for (const auto& state : automata.outputs) {
        file << ";" << state.first;
    }
    file << endl;
    // inputs and transitions
    int inputIndex = 0;
    auto statesQual = automata.states.size();
    for (const string& input : automata.inputs) {
        file << input << ";";
        for (int stateIndex = 0; stateIndex < automata.states.size(); stateIndex++) {
            string nextState = automata.transitions[inputIndex][stateIndex];
            file << nextState;
            if (stateIndex != automata.states.size() - 1) {
                file << ";";
            }
        }
        file << endl;
        inputIndex++;
    }
    file.close();
}

void exportMealyToCSV(MealyAutomata automata, const string& filename) {
    ofstream file(filename);
    if (!file.is_open()) {
        cerr << "Failed to open file: " << filename << endl;
        return;
    }
    for (const auto& state : automata.states) {
        file << ";" << state;
    }
    file << endl;
    int inputIndex = 0;
    auto statesQual = automata.states.size();
    for (const string& input : automata.inputs) {
        file << input << ";";
        for (int stateIndex = 0; stateIndex < automata.states.size(); stateIndex++) {
            auto nextState = automata.transitions[inputIndex][stateIndex];
            file << nextState.first << "/" << nextState.second;
            if (stateIndex != automata.states.size() - 1) {
                file << ";";
            }
        }
        file << endl;
        inputIndex++;
    }
    file.close();
}

MooreAutomata RemoveUnreachableStatesMoore(MooreAutomata& automata) {
    unordered_set<string> reachableStates;
    queue<string> toVisit;

    // Начинаем с начального состояния
    string startState = *automata.states.begin();
    toVisit.push(startState);
    reachableStates.insert(startState);

    // Обход в ширину для поиска всех достижимых состояний
    while (!toVisit.empty()) {
        string currentState = toVisit.front();
        toVisit.pop();

        // Находим индекс текущего состояния
        auto stateIt = find(automata.states.begin(), automata.states.end(), currentState);
        if (stateIt == automata.states.end()) {
            cerr << "Error: State " << currentState << " not found in states set." << endl;
            continue;
        }
        int stateIndex = distance(automata.states.begin(), stateIt);

        // Проходим по всем входным символам
        for (size_t i = 0; i < automata.transitions.size(); ++i) {
            try {
                string nextState = automata.transitions.at(i).at(stateIndex);
                if (reachableStates.find(nextState) == reachableStates.end()) {
                    reachableStates.insert(nextState);
                    toVisit.push(nextState);
                }
            }
            catch (const out_of_range& e) {
                cerr << "Error: Out of range access in transitions matrix." << endl;
            }
        }
    }

    // Удаляем недостижимые состояния
    for (auto it = automata.states.begin(); it != automata.states.end();) {
        if (reachableStates.find(*it) == reachableStates.end()) {
            it = automata.states.erase(it);
        }
        else {
            ++it;
        }
    }

    // Удаляем переходы в недостижимые состояния
    for (auto& transitions : automata.transitions) {
        for (auto it = transitions.begin(); it != transitions.end();) {
            if (reachableStates.find(*it) == reachableStates.end()) {
                it = transitions.erase(it);
            }
            else {
                ++it;
            }
        }
    }

    // Удаляем выходные символы для недостижимых состояний
    for (auto it = automata.outputs.begin(); it != automata.outputs.end();) {
        if (reachableStates.find(it->first) == reachableStates.end()) {
            it = automata.outputs.erase(it);
        }
        else {
            ++it;
        }
    }

    MooreAutomata ProcAut = automata;
    return ProcAut;
}

MealyAutomata RemoveUnreachableStatesMealy(MealyAutomata& automata)
{
    unordered_set<string> reachableStates;
    queue<string> toVisit;

    // Начинаем с начального состояния
    string startState = *automata.states.begin();
    toVisit.push(startState);
    reachableStates.insert(startState);

    // Обход в ширину для поиска всех достижимых состояний
    while (!toVisit.empty()) {
        string currentState = toVisit.front();
        toVisit.pop();

        // Находим индекс текущего состояния
        auto stateIt = find(automata.states.begin(), automata.states.end(), currentState);
        if (stateIt == automata.states.end()) {
            cerr << "Error: State " << currentState << " not found in states set." << endl;
            continue;
        }
        int stateIndex = distance(automata.states.begin(), stateIt);

        // Проходим по всем входным символам
        for (size_t i = 0; i < automata.transitions.size(); ++i) {
            try {
                string nextState = automata.transitions.at(i).at(stateIndex).first;
                if (reachableStates.find(nextState) == reachableStates.end()) {
                    reachableStates.insert(nextState);
                    toVisit.push(nextState);
                }
            }
            catch (const out_of_range& e) {
                cerr << "Error: Out of range access in transitions matrix." << endl;
            }
        }
    }

    // Удаляем недостижимые состояния
    for (auto it = automata.states.begin(); it != automata.states.end();) {
        if (reachableStates.find(*it) == reachableStates.end()) {
            it = automata.states.erase(it);
        }
        else {
            ++it;
        }
    }

    for (auto& transitions : automata.transitions) {
        for (auto it = transitions.begin(); it != transitions.end();) {
            string state = it->first;
            if (reachableStates.find(state) == reachableStates.end()) {
                it = transitions.erase(it);
            }
            else {
                ++it;
            }
        }
    }
    MealyAutomata ProcAut = automata;
    return ProcAut;
}

MealyAutomata ConvertMooreToMealy(MooreAutomata moore) {
    MealyAutomata mealy;

    mealy.states = moore.states;
    mealy.inputs = moore.inputs;

    mealy.transitions.resize(moore.inputs.size(), std::vector<std::pair<std::string, std::string>>(moore.states.size()));
    std::unordered_map<std::string, int> stateIndexMap;
    int index = 0;
    for (const auto& state : moore.states) {
        stateIndexMap[state] = index;
        index++;
    }   
    // Заполняем таблицу переходов автомата Мили
    for (int inputIndex = 0; inputIndex < moore.inputs.size(); inputIndex++) {
        for (int stateIndex = 0; stateIndex < moore.states.size(); stateIndex++) {
            // Находим следующее состояние по текущему состоянию и входу
            string nextState = moore.transitions[inputIndex][stateIndex];
            if (!nextState.empty() && nextState != " ")
            {
               string output = moore.outputs.at(nextState);
               mealy.transitions[inputIndex][stateIndex] = { nextState, output };
            }
        }
    }
    return mealy;
}

MooreAutomata ConvertMealyToMoore(MealyAutomata mealy)
{
    MooreAutomata moore;

    moore.inputs = mealy.inputs;
    // {MealyState, outputs}
    map<string, set<string>> statesCard;
    for (int inputIndex = 0; inputIndex < moore.inputs.size(); inputIndex++) {    
        for (int stateIndex = 0; stateIndex < mealy.states.size(); stateIndex++) {
            pair<string, string> nextStateAndOut = mealy.transitions[inputIndex][stateIndex];
            statesCard[nextStateAndOut.first].insert(nextStateAndOut.second);
        }
    }
    int mooreStateNum = 0;
    // {{ MealyState, output }, MooreState}
    unordered_map<pair<string, string>,string, pair_hash> NewStatesCard;
    for (auto it = statesCard.begin(); it != statesCard.end(); ++it) {
        string state = it->first;
        for (const auto& output : it->second) {
            string newState = MOORE_STATE_CH + std::to_string(mooreStateNum);
            pair<string, string> transition = { state, output };
            NewStatesCard[transition] = newState;
            moore.outputs[newState] = output;
            moore.states.insert(newState);
            mooreStateNum++;
        }
    }
    // окончательная таблица
    moore.transitions.resize(moore.inputs.size());
    int inputIndex = 0;
    for (int inputIndex = 0; inputIndex < mealy.transitions.size(); inputIndex++) {
        int transitionIndex = 0;      
        for (const auto& state : statesCard) {
            string nextMealyState = mealy.transitions[inputIndex][transitionIndex].first;
            for (const auto& outputs : state.second) {
                moore.transitions[inputIndex].push_back(NewStatesCard[mealy.transitions[inputIndex][transitionIndex]]);
            }
            transitionIndex++;
        }
    }
    return moore;
}

int main()
{
    MooreAutomata aut = ReadMoore(inputFile);
    aut = RemoveUnreachableStatesMoore(aut);
    MealyAutomata mealyAut = ConvertMooreToMealy(aut);
    exportMealyToCSV(mealyAut, outputFile);
    //MealyAutomata mealyAut = ReadMealy(secondFile);
    //mealyAut = RemoveUnreachableStatesMealy(mealyAut);
    ///*PrintMealyAutomata(mealyAut);*/
    //MooreAutomata mooreAut = ConvertMealyToMoore(mealyAut);
    //exportMooreToCSV(mooreAut, outputFile);
    /*PrintMooreAutomata(mooreAut);*/
	return 0;
}
