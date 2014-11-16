#!/usr/bin/python

def main():
    import sys

    if sys.version_info[:2] < (2, 7):
        sys.exit("ERROR: Requires Python >= 2.7")

    # let's read in some command-line arguments
    args = parse_cli()

    # create output file
    if args.textFile is None:
        sys.exit("ERROR: Need input training text file.") 
    
    trainTextFile = open(args.textFile, 'r')
    trainText = trainTextFile.read()
    markovMatrix = createMarkovMatrix(trainText)
    printMarkovMatrix(markovMatrix, args.markovOut)
def parse_cli():
    '''
    parse command-line interface arguments
    '''
    import argparse

    parser = argparse.ArgumentParser(description="Makes k-th order Markov mapping array for "
                                     "lowercase letters in a given text file.",
                                     formatter_class=
                                     argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument("--textFile",
                        help="name of ASCII file training text.",
                        default="training.txt")
    parser.add_argument("--markovOut", help="name of output markovMatrix file.",
                        default="markovOut.txt")
    args = parser.parse_args()

    return args
 
def createMarkovMatrix(trainText):
    trainText = trainText.lower()
    markovMatrix = [[0 for i in range(26)] for i in range(26)]
    wrapAroundText = trainText + trainText[0]
    
    for charCount in range (1, len(trainText)):
        currentChar = trainText[charCount-1]
        nextChar = trainText[charCount] 
        if currentChar.isalpha() and nextChar.isalpha():
            markovMatrix[ord(currentChar)-ord('a')][ord(nextChar)-ord('a')] += 1;

    return markovMatrix
    
def printMarkovMatrix(markovMatrix, markovOutFile):
    markovMatrixFile = open(markovOutFile, 'w')
    for ascii in range(97, 123):
        markovMatrixFile.write(chr(ascii) +  ' ')
        #print markovMatrix[ascii-97]
        for i in range(26):
            markovMatrix[ascii-97][i] = str(markovMatrix[ascii-97][i])
        markovMatrixFile.write(' '.join(markovMatrix[ascii-97]))
        markovMatrixFile.write('\n')
if __name__ == "__main__":
    main()