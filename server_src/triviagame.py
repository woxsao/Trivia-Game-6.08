import sqlite3

def request_handler(request):
    """
    Our get request in this case returns the html list for the scoreboard. The post request returns user specific score and updates it with the increment amount.
    """
    visits_db = '/var/jail/home/mochan/trivia/triviaboard.db'
    conn = sqlite3.connect(visits_db)  # connect to that database (will create if it doesn't already exist)
    c = conn.cursor()  # move cursor into database (allows us to execute commands)
    outs = ""
    c.execute('''CREATE TABLE IF NOT EXISTS scoreboard (user text UNIQUE, correct int, incorrect int, score int);''') # run a CREATE TABLE command
    
    if request["method"] == "GET":
        try:
            scoreboard = bool(request['values']['scoreboard'])
        except Exception as e:
            return "missing scoreboard parameter"
        if scoreboard == True:
            elements = c.execute('''SELECT * FROM scoreboard ORDER BY score DESC;''').fetchall()
            for row in elements:
                row = list(row)
        
        res = "<ol>\n"
        for s in elements:
            res += "<li> {user}:    correct: {correct}, incorrect: {incorrect}, NET SCORE: {score} </li>\n".format(user = s[0], correct = s[1], incorrect = s[2], score = s[3])
        res += "</ol>\n"
        conn.commit() # commit commands
        conn.close()
        return res
    else: #Post Request
        try:
            user = request['form']['user']
            correct= int(request['form']['correct'])
            incorrect = int(request['form']['incorrect'])
        except Exception as e:
            return "Error: one or more arguments are missing/invalid"
       
        c.execute('''INSERT OR IGNORE into scoreboard (user, correct, incorrect, score) VALUES (?,?,?,?);''', (user, 0,0,0))
        c.execute('''UPDATE scoreboard SET correct = correct + ? WHERE user = ?;''', (correct, user)) #increment the values, don't add a new entry.
        c.execute('''UPDATE scoreboard SET incorrect = incorrect + ? WHERE user = ?;''', (incorrect, user)) #increment the values, don't add a new entry.
        #score = correct minus incorrect
        c.execute('''UPDATE scoreboard SET score = correct - incorrect WHERE user = ?;''', (user,)) #increment the values, don't add a new entry.
        user_input = c.execute('''SELECT * FROM scoreboard where user = ?;''', (user,)).fetchall()
        for row in user_input:
            row = list(user_input)

        #my return value is just a string with the user data. 
        res = "correct: {correct}, incorrect: {incorrect}, score: {score}".format(correct = user_input[0][1], incorrect = user_input[0][2], score = user_input[0][3])

        conn.commit() # commit commands
        conn.close() # close connection to database
        return res