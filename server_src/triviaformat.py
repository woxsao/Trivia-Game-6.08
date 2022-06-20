def request_handler(request):
    """
    This function basically just formats the raw question into something prettier by cleaning up the unicode into proper apostrophes/quotes.
    Parameters:
    *request is a dictionary with our request.
    """
    if request["method"] == "GET":
        pass
    elif request["method"] == "POST":
        question = str(request['data'])
        formatted_question = question.replace("&#039;", "\'")
        formatted_question = formatted_question.replace("&quot;","\"")
        formatted_question = formatted_question.replace("&ldquo;","\"")
        formatted_question = formatted_question.replace("&rsquo;", "\'")
        return formatted_question
