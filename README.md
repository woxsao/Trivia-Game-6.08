# Trivia Game 👩🏻‍💻❓
## Monica Chan (mochan@mit.edu)

[demo video] (https://youtu.be/9Njs3h-Shh0)
## Features Implemented

- HTML Table Scoreboard Viewer that displays user's all time score
- True/False questions in rounds of 10 questions


## List of Functions (more detailed descriptions can be found in the code)
#### Ones given:
- char_append (used in the http get function to append to the response buffer)
- do_http_request (performs an http post/get request)
- do_https_request (performs a secure request)

#### New ones:
- yield_question: Displays the question to the screen and also the score. Takes user input and updates the score on the database accordingly. also formats the question to make it pretty (i.e. correctly displaying quotes and apostrophes) by performing a Post request to triviaformat.py
- answer_question: Handles parsing the user inputs (on the buttons)
- generate_questions: performs a get request to the trivia server to get a set of true/false questions and deserializes the response into a json doc.
- sm_45: State machine for button at pin 45 (the button that represents a "true" answer)
- sm_39: State machine for button at pin 39 (the button that represents a "false" answer)
- post_score: Performs a post request to my triviagame.py to update the score of the user. 
- run_round: puts all the moving parts together (post_score, generate_questions, yield_questions)

#### Server Files:
- triviaformat: A Post request to this script formats the question to make it pretty (formatting apostrophes/quotes)
- triviagame: A post request to this script updates the user's score and returns what the user's new score, # correct, and # incorrect questions are. A get request will return an HTML list that can be displayed in the browser if scoreboard = True is a parameter in the url. 
    - The database table contains the (user string, #correct int, #incorrect int, score = #correct - #incorrect int)  

### General Structure/Tracing What Happens:
```cpp
void run_round(){
  if(strlen(user_score)==0){ //if we don't have data about the user score (just on startup), do an empty post request to get back the data.
    post_score(0,0); 
    
  }
  generate_questions();
  for(int i = 0; i < 10; i++){
    just_answered = 0;
    yield_question(i);
  }
}
```
- run_round runs all of the functions:
- If we've just started up the system, there is nothing inside the user_score string, so we can perform an empty post request to triviagame.py (by running post_score(0,0)). This means we're doing a request by incrementing the number of incorrect/correct questions by 0.
- We then generate the 10 questions.
- Run inside the for loop yielding each question individually.
- Inside yield_question, we already have a nice deserialized doc item. We do a post request to triviaformat to prettify the question, and then display it to the tft. We also display the current user_score (retrieved from triviagame script) to the screen. At the very end of each yield_question call, we increment the score by running post_score accordingly. 

### Important Design notes/Thinking
- If the https doesn't work, go to lines 237 and 240 and uncomment those. I don't think the https should break since I found the root cert, but just in case it does. 
- I had lots of (not) fun performing the https request. Because trivia database requires a secure connection, I had two options: using python or tracking down the root certificate. I chose the latter, and learned a lot about how to retrieve the certificate from Chrome. I then use the do_https_request function with the cert. 
- Another note was that I discovered you could actually get away with using do_http_request, just editing the host line of the get request to look like: 
```cpp
strcat(request_buffer,"Host: opentdb.com:443\r\n");
```
Surprisingly, this works.
- This was likely undesired behavior, so I got rid of it and hunted down the cert. 

