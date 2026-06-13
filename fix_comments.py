import os

with open("lcs/lcs_app.c", "r") as f:
    lines = f.readlines()

out_lines = []
in_block = False
block_lines = []

def parse_and_format(block):
    func_name = ""
    returns = ""
    reference = ""
    purpose = ""
    comments = ""
    
    current_key = None
    
    for line in block:
        line_clean = line.strip()
        if not line_clean:
            continue
        if line_clean.startswith("Function:"):
            func_name = line_clean.replace("Function:", "").strip()
            current_key = "func"
        elif line_clean.startswith("Returns:"):
            returns = line_clean.replace("Returns:", "").strip()
            current_key = "returns"
        elif line_clean.startswith("Reference:"):
            reference = line_clean.replace("Reference:", "").strip()
            current_key = "reference"
        elif line_clean.startswith("Purpose:"):
            purpose = line_clean.replace("Purpose:", "").strip()
            current_key = "purpose"
        elif line_clean.startswith("Comments:"):
            comments = line_clean.replace("Comments:", "").strip()
            current_key = "comments"
        elif line_clean.startswith("Comment:"):
            comments = line_clean.replace("Comment:", "").strip()
            current_key = "comments"
        else:
            if current_key == "func":
                func_name += " " + line_clean
            elif current_key == "returns":
                returns += " " + line_clean
            elif current_key == "reference":
                reference += " " + line_clean
            elif current_key == "purpose":
                purpose += " " + line_clean
            elif current_key == "comments":
                comments += " " + line_clean
                
    new_comment = ["/*\n"]
    if purpose and purpose != "None.":
        new_comment.append(f" * {purpose}\n")
    elif func_name:
        new_comment.append(f" * {func_name}\n")
        
    new_comment.append(" * Parameters:\n")
    new_comment.append(" *   None\n") 
    
    if returns:
        new_comment.append(" * Returns:\n")
        new_comment.append(f" *   {returns}\n")
        
    notes = []
    if comments and comments != "None." and comments != "None":
        notes.append(comments)
    if reference and reference != "None." and reference != "None":
        notes.append(f"Reference: {reference}")
        
    if notes:
        new_comment.append(" * Notes:\n")
        for note in notes:
            new_comment.append(f" *   {note}\n")
            
    new_comment.append(" */\n")
    return new_comment

i = 0
while i < len(lines):
    line = lines[i]
    if line.startswith("/*******************************************************************************") and \
       (i + 1 < len(lines)) and lines[i+1].startswith("Function:"):
        # found block
        block = []
        i += 1
        while i < len(lines) and not lines[i].startswith("*******************************************************************************/"):
            block.append(lines[i])
            i += 1
        
        # we are at the closing asterisks
        out_lines.extend(parse_and_format(block))
    else:
        out_lines.append(line)
    i += 1

with open("lcs/lcs_app.c", "w") as f:
    f.writelines(out_lines)
