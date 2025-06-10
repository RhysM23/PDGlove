import tkinter as tk
from tkinter import ttk, scrolledtext

def create_tab(notebook, title):
    """Create a tab in the notebook with the given title"""
    tab = ttk.Frame(notebook)
    notebook.add(tab, text=title)
    return tab

def create_logger(parent):
    """Create a scrollable log text area"""
    log_text = scrolledtext.ScrolledText(parent, height=10)
    log_text.pack(fill='both', expand=True)
    log_text.config(state='disabled')
    return log_text