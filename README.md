# SEP-UG-33
SEP Project Github

Github basic rules:

**Work in seperate branch pulled from main linked to a ticket, spaces in ticket name seperated by hypen.**
  UG33-ticketnumber-ticketname

  eg.
  Project Name: UG33 
  Ticket number: 19 
  Ticket Name: Initiate Group Kick Off Meeting 
  
  Turns into branch name:
  UG33-19-initiate-group-kick-off-meeting

**When trying to push to main, create a pull request.**
  Resolve merge conflicts by rebasing to main (more relevant to end).

**Commit according to conventional commits:**
  https://www.conventionalcommits.org/en/v1.0.0/

  try your best lol basic summary:
  most of ours will be feat/chore.

            feat: new feature
            fix: bug fix
            docs: documentation only
            refactor: code change, no new feature or fix
            chore: maintenance tasks (build, dependencies)
        
          examples:
          
              feat: add Google login support
              chore: update React to v18
  
**Pull request approved by someone other than branch-owner before push to main (shows evidence of acutal SWE work).**

** potentially setting up pipeline checks to ensure **
