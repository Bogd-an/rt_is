# Робототехніка та інтелектуальні системи


``` shell
winget install  Python.Python.3.10
python3.10 -m venv .venv
.venv/Scripts/Activate.ps1
pip install -r requirements.txt
```


------------------------------------------------------------------------------------------------

Якщо свариться на політику, то можна зробити дирку в безпеці своїми руками в PowerShell admin
`Set-ExecutionPolicy -Scope CurrentUser -ExecutionPolicy RemoteSigned`

``` shell
cannot be loaded because running scripts is disabled on this system. For more informat
see about_Execution_Policies at https:/go.microsoft.com/fwlink/?LinkID=135170.
    + CategoryInfo          : SecurityError: (:) [], PSSecurityException
    + FullyQualifiedErrorId : UnauthorizedAccess
```
