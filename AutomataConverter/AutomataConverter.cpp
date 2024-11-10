#include "AutomataConverter.h"

using namespace std;
const string MEALY_TO_MOORE_PARAM = "mealy-to-moore";
const string MOORE_TO_MEALY_PARAM = "moore-to-mealy";
const string BLANK_OUTPUT_CH = "_";
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
    vector<string> statesTable;
    vector<string> inputs;
};

struct MealyAutomata {
    vector<vector<pair<string, string>>> transitions;
    vector<string> statesTable;
    vector<string> inputs;
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
        aut.statesTable.push_back(state);
        aut.outputs[state] = output_symbols[stateIndex];
        stateIndex++;
    }
    // Чтение переходов
    while (getline(file, line))
    {
        stringstream ss(line);
        string input, transition;
        getline(ss, input, ';');
        aut.inputs.push_back(input);
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
        mealyAutomata.statesTable.push_back(state);
    }
    // Чтение переходов
    while (getline(file, line)) {
        stringstream ss(line);
        string input, transition;
        getline(ss, input, ';');
        mealyAutomata.inputs.push_back(input);
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

void ExportMooreToCSV(MooreAutomata automata, const string& filename) {
    ofstream file(filename);
    if (!file.is_open()) {
        cerr << "Failed to open file: " << filename << endl;
        return;
    }
    // outputs
    for (const auto& state : automata.statesTable) {
        if (automata.outputs[state] == BLANK_OUTPUT_CH) {
            file << ";";
        }
        else
        {
            file << ";" << automata.outputs[state];
        }    
    }
    file << endl;
    for (const auto& state : automata.statesTable) {
        file << ";" << state;
    }
    file << endl;
    // inputs and transitions
    int inputIndex = 0;
    auto statesQual = automata.statesTable.size();
    for (const string& input : automata.inputs) {
        file << input << ";";
        for (int stateIndex = 0; stateIndex < automata.statesTable.size(); stateIndex++) {
            string nextState = automata.transitions[inputIndex][stateIndex];
            file << nextState;
            if (stateIndex != automata.statesTable.size() - 1) {
                file << ";";
            }
        }
        file << endl;
        inputIndex++;
    }
    file.close();
}

void ExportMealyToCSV(MealyAutomata automata, const string& filename) {
    ofstream file(filename);
    if (!file.is_open()) {
        cerr << "Failed to open file: " << filename << endl;
        return;
    }
    for (const auto& state : automata.statesTable) {
        file << ";" << state;
    }
    file << endl;
    int inputIndex = 0;
    auto statesQual = automata.statesTable.size();
    for (const string& input : automata.inputs) {
        file << input << ";";
        for (int stateIndex = 0; stateIndex < automata.statesTable.size(); stateIndex++) {
            auto nextState = automata.transitions[inputIndex][stateIndex];
            file << nextState.first << "/" << nextState.second;
            if (stateIndex != automata.statesTable.size() - 1) {
                file << ";";
            }
        }
        file << endl;
        inputIndex++;
    }
    file.close();
}

MooreAutomata RemoveUnreachableStatesMoore(MooreAutomata automata) {
    unordered_set<string> reachableStates;
    queue<string> toVisit;
    MooreAutomata procAut;
    procAut.outputs = automata.outputs;
    procAut.inputs = automata.inputs;
    procAut.transitions.resize(procAut.inputs.size());

    // Начинаем с начального состояния
    string startState = automata.statesTable[0];
    toVisit.push(startState);
    reachableStates.insert(startState);

    // Обход в ширину для поиска всех достижимых состояний
    while (!toVisit.empty()) {
        string currentState = toVisit.front();
        toVisit.pop();
        int stateIndex = 0;
        // Находим индекс текущего состояния
        for (const auto& state : automata.statesTable)
        {
            if (currentState == state)
            {
                break;
            }
            stateIndex++;
        }
        // Проходим по всем входным символам
        for (size_t i = 0; i < automata.transitions.size(); i++) {
            if (stateIndex >= automata.transitions[i].size()) {
                cerr << "Error: State index out of range in transitions matrix." << endl;
                continue;
            }
            string nextState = automata.transitions[i][stateIndex];
            if (reachableStates.find(nextState) == reachableStates.end()) {
                reachableStates.insert(nextState);
                toVisit.push(nextState);
            }
        }
    }

    // Удаляем недостижимые состояния
    vector<pair<string, int>>unreachableStatesWithIndex;
    int currentStateIndex = 0;
    for (const auto& state : automata.statesTable) {
        if (reachableStates.find(state) == reachableStates.end()) {
            unreachableStatesWithIndex.push_back({ state, currentStateIndex });
        }
        currentStateIndex++;
    }
    currentStateIndex = 0;
    for (const auto& state : unreachableStatesWithIndex) {
        procAut.outputs.erase(state.first);
        for (int stateIndex = currentStateIndex; stateIndex < automata.statesTable.size(); stateIndex++) {
            currentStateIndex++;
            if (automata.statesTable[stateIndex] == state.first) {
                break;
            }
            else {
                procAut.statesTable.push_back(automata.statesTable[stateIndex]);
                for (int input = 0; input < automata.transitions.size(); input++) {
                    procAut.transitions[input].push_back(automata.transitions[input][stateIndex]);
                }
            }
        }
    }
    if (currentStateIndex < automata.statesTable.size()) {
        for (int stateIndex = currentStateIndex; stateIndex < automata.statesTable.size(); stateIndex++) {
            procAut.statesTable.push_back(automata.statesTable[stateIndex]);
            for (int input = 0; input < automata.transitions.size(); input++) {
                procAut.transitions[input].push_back(automata.transitions[input][stateIndex]);
            }
        }
    }

    return procAut;
}

MealyAutomata RemoveUnreachableStatesMealy(MealyAutomata automata)
{
    unordered_set<string> reachableStates;
    queue<string> toVisit;
    MealyAutomata procAut;
    procAut.inputs = automata.inputs;
    procAut.transitions.resize(procAut.inputs.size());

    // Начинаем с начального состояния
    string startState = automata.statesTable[0];
    toVisit.push(startState);
    reachableStates.insert(startState);

    // Обход в ширину для поиска всех достижимых состояний
    while (!toVisit.empty()) {
        string currentState = toVisit.front();
        toVisit.pop();

        int stateIndex = 0;
        // Находим индекс текущего состояния
        for (const auto& state : automata.statesTable)
        {
            if (currentState == state)
            {
                break;
            }
            stateIndex++;
        }

        // Проходим по всем входным символам
        for (size_t i = 0; i < automata.transitions.size(); i++) {
            if (stateIndex >= automata.transitions[i].size()) {
                cerr << "Error: State index out of range in transitions matrix." << endl;
                continue;
            }
            string nextState = automata.transitions[i][stateIndex].first;
            if (reachableStates.find(nextState) == reachableStates.end()) {
                reachableStates.insert(nextState);
                toVisit.push(nextState);
            }
        }
    }

    // Удаляем недостижимые состояния
    vector<pair<string, int>>unreachableStatesWithIndex;
    int currentStateIndex = 0;
    for (const auto& state : automata.statesTable) {
        if (reachableStates.find(state) == reachableStates.end()) {
            unreachableStatesWithIndex.push_back({ state, currentStateIndex });
        }
        currentStateIndex++;
    }
    currentStateIndex = 0;
    for (const auto& state : unreachableStatesWithIndex) {
        for (int stateIndex = currentStateIndex; stateIndex < automata.statesTable.size(); stateIndex++) {
            currentStateIndex++;
            if (automata.statesTable[stateIndex] == state.first) {
                break;
            }
            else {
                procAut.statesTable.push_back(automata.statesTable[stateIndex]);
                for (int input = 0; input < automata.transitions.size(); input++) {
                    procAut.transitions[input].push_back(automata.transitions[input][stateIndex]);
                }
            }
        }
    }
    if (currentStateIndex < automata.statesTable.size()) {
        for (int stateIndex = currentStateIndex; stateIndex < automata.statesTable.size(); stateIndex++) {
            procAut.statesTable.push_back(automata.statesTable[stateIndex]);
            for (int input = 0; input < automata.transitions.size(); input++) {
                procAut.transitions[input].push_back(automata.transitions[input][stateIndex]);
            }
        }
    }
    return procAut;
}

MealyAutomata ConvertMooreToMealy(MooreAutomata moore) {
    MealyAutomata mealy;

    mealy.statesTable = moore.statesTable;
    mealy.inputs = moore.inputs;

    mealy.transitions.resize(moore.inputs.size(), std::vector<std::pair<std::string, std::string>>(moore.statesTable.size()));
    std::unordered_map<std::string, int> stateIndexMap;
    int index = 0;
    for (const auto& state : moore.statesTable) {
        stateIndexMap[state] = index;
        index++;
    }
    // Заполняем таблицу переходов автомата Мили
    for (int inputIndex = 0; inputIndex < moore.inputs.size(); inputIndex++) {
        for (int stateIndex = 0; stateIndex < moore.statesTable.size(); stateIndex++) {
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

MooreAutomata AltConvertMealyToMoore(MealyAutomata mealy)
{
    MooreAutomata moore;

    moore.inputs = mealy.inputs;
    // {MealyState, transitions}
    map<string, vector<pair<string, string>>> transitionTable;
    // {MealyState, outputs}
    map<string, set<string>> statesCard;
    bool needNewStates = false;
    for (int stateIndex = 0; stateIndex < mealy.statesTable.size(); stateIndex++) {
        for (int inputIndex = 0; inputIndex < moore.inputs.size(); inputIndex++) {
            pair<string, string> nextStateAndOut = mealy.transitions[inputIndex][stateIndex];
            statesCard[nextStateAndOut.first].insert(nextStateAndOut.second);
            transitionTable[mealy.statesTable[stateIndex]].push_back(nextStateAndOut);
            if (statesCard[nextStateAndOut.first].size() > 1) {
                needNewStates = true;
            }
        }
    }
    int mooreStateNum = 0;
    // {{ MealyState, output }, MooreState}
    unordered_map<pair<string, string>, string, pair_hash> NewStatesCard;
    vector<string> orderedMealyStates;
    // назначаем для первого состояния
    auto firstState = mealy.statesTable[0];
    if (!needNewStates) {
        moore.statesTable = mealy.statesTable;
        orderedMealyStates = mealy.statesTable;
    }
    else {
        orderedMealyStates.push_back(firstState);
    }
    if (statesCard.find(firstState) == statesCard.end()) {
        statesCard[firstState].insert(BLANK_OUTPUT_CH);
    }
    for (const auto& output : statesCard[firstState]) {
        string newState;
        if (needNewStates) {
            newState = MOORE_STATE_CH + std::to_string(mooreStateNum);
            moore.statesTable.push_back(newState);
        }
        else {
            newState = firstState;
        }
        pair<string, string> transition = { firstState, output };
        NewStatesCard[transition] = newState;
        moore.outputs[newState] = output;
        mooreStateNum++;
    }
    moore.transitions.resize(moore.inputs.size());
    int transitionIndex = 0;
    for (int transitionIndex = 0; transitionIndex < mealy.transitions[0].size(); transitionIndex++)
    {
        //cout << transitionIndex << "\n";
        for (int input = 0; input < mealy.inputs.size(); input++) {
            auto next = mealy.transitions[input][transitionIndex];
            // проверка, что не присвоили новый state раньше
            if (NewStatesCard.find(next) == NewStatesCard.end()) {
                if (needNewStates) {
                    orderedMealyStates.push_back(next.first);
                    for (const auto& output : statesCard[next.first]) {
                        string newState = MOORE_STATE_CH + std::to_string(mooreStateNum);
                        pair<string, string> transition = { next.first, output };
                        NewStatesCard[transition] = newState;
                        moore.outputs[newState] = output;
                        moore.statesTable.push_back(newState);
                        mooreStateNum++;
                    }
                }
                else {
                    string state = next.first;
                    pair<string, string> transition = { next.first, next.second };
                    NewStatesCard[transition] = state;
                    moore.outputs[state] = next.second;
                }
            }
        }
    }
    for (auto& mealyState : orderedMealyStates) {
        int input = 0;
        for (auto& transition : transitionTable[mealyState]) {
            for (auto& output : statesCard[mealyState]) {
                moore.transitions[input].push_back(NewStatesCard[transition]);
            }
            input++;
        }
    }
    return moore;
}

void PrintMooreAutomata(MooreAutomata automata) {
    cout << "Outputs:" << endl;
    for (const auto& pair : automata.outputs) {
        cout << pair.first << " -> " << pair.second << endl;
    }

    cout << "\nStates:" << endl;
    for (const auto& state : automata.statesTable) {
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
            cout << "State: " << *next(automata.statesTable.begin(), j) << " -> " << automata.transitions[i][j] << "\n";
        }
    }
}

void PrintMealyAutomata(MealyAutomata mealyAutomata) {
    cout << "States:" << endl;
    for (const auto& state : mealyAutomata.statesTable) {
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
            cout << "State: " << *next(mealyAutomata.statesTable.begin(), j) << " -> " << mealyAutomata.transitions[i][j].first << "/" << mealyAutomata.transitions[i][j].second << endl;
        }
    }
}

int main(int argc, char* argv[])
{
    if (argc != 4) {
        cerr << "Usage: " << "<work param> <input_file> <output_file>" << endl;
        return 1;
    }
    string workParam = argv[1];
    string inputFile = argv[2];
    string outputFile = argv[3];
    if (workParam != MEALY_TO_MOORE_PARAM && workParam != MOORE_TO_MEALY_PARAM)
    {
        cerr << "Wrong param" << endl;
        return 1;
    }
    if (workParam == MEALY_TO_MOORE_PARAM) {
        MealyAutomata mealyAut = ReadMealy(inputFile);
        mealyAut = RemoveUnreachableStatesMealy(mealyAut);
        PrintMealyAutomata(mealyAut);
        MooreAutomata mooreAut = AltConvertMealyToMoore(mealyAut);
        ExportMooreToCSV(mooreAut, outputFile);
    }
    else
    {
        if (workParam == MOORE_TO_MEALY_PARAM) {
            MooreAutomata aut = ReadMoore(inputFile);
            aut = RemoveUnreachableStatesMoore(aut);
            MealyAutomata mealyAut = ConvertMooreToMealy(aut);
            ExportMealyToCSV(mealyAut, outputFile);
        }
    }
    return 0;
}
