char *progScanner(FILE *inputFile, char *inputLine){
   //////////USED FOR TESTING///////
//  char *inputLine;
// inputLine=(char*)malloc(100*sizeof(char ));
//   fgets(inputLine, 100, stdin);
//////////////////////////////////////


    fgets(inputLine,100, inputFile) 
    char *givenLine;
    givenLine = (char*)malloc(100*sizeof(char ));
    strcpy(givenLine,inputLine); //strtok is destructive so im using a copy instead

    int i;
    char delimiters[]={"," "(" ")" ";" "\n" " "};  
    char ** instructionFields;

    instructionFields = (char **)malloc(100*sizeof(char *));
    for (i=0; i<4; i++)
       *(instructionFields+i) = (char *) malloc(20*sizeof(char *));

    instructionFields[0] = strtok(givenLine, delimiters);

    if(strcmp(instructionFields[0],"haltSimulation")== 0)
    {

        for(i=1;i<4;i++)
            instructionFields[i] = "0";

    }
    else
    {

        for(i=1;i<4;i++)
            instructionFields[i]=strtok(NULL,delimiters); 

    }

    char *result = malloc(100*sizeof(char *));
    strcpy(result, instructionFields[0]);
    strcat(result, " ");
    strcat(result, instructionFields[1]);
    strcat(result, " ");
    strcat(result, instructionFields[2]);
    strcat(result, " ");
    strcat(result, instructionFields[3]);
    
    printf("%s\n",result);

}

